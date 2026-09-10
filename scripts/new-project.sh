#!/usr/bin/env bash
# Create a project from projects/template, optionally with more than one
# application.
#
#   ./scripts/new-project.sh myproject          # app0 only
#   ./scripts/new-project.sh myproject 2        # app0, app1, app2
#
# The second argument is the highest application index, so 0 means app0 alone
# and 2 means app0, app1 and app2. Every generated application gets its own
# linker script, its own main.c with the trap handler installed, and an entry
# in the project Makefile's APPS. app0 is generated with the monitor calls that
# hand each of the others its memory region and a UART frame; the time slice
# and the resume are left commented out, so a fresh project builds and runs
# with only app0 executing.
#
# Invoked by `make new NAME=<name> APPS=<n>` from the repository root.

set -euo pipefail

prog=${0##*/}
root=$(cd -- "$(dirname -- "$0")/.." && pwd)
cd "$root"

template=${TEMPLATE:-projects/template}

# Matches INIT_CAPS: app0 runs from the boot PMP frame at 0x80010000, and
# RAM_MEM starts at 0x80020000, so app1 and up are carved from RAM_MEM.
APP0_BASE=0x80010000
APP_SIZE=0x10000

# Matches S3K_SLOT_CNT in the generated s3k_conf.h. Used only to pick the
# commented-out time slices.
SLOT_CNT=32

# One process per application, and each needs a slot range, so the slice for
# the last one must not round down to zero.
MAX_APPS=31

die() {
	echo "${prog}: $*" >&2
	exit 1
}

usage() {
	cat >&2 <<EOF
usage: ${prog} <name> [apps]

  <name>   project name; creates projects/<name>
  [apps]   highest application index (default 0)
             0 -> app0
             1 -> app0 app1
             n -> app0 .. app<n>        (n at most ${MAX_APPS})
EOF
	exit 1
}

if [ $# -lt 1 ] || [ $# -gt 2 ]; then
	usage
fi

name=$1
apps=${2:-0}

case "$name" in
*/* | . | ..) die "'${name}' is not a valid project name" ;;
esac
case "$name" in
'' | *[!A-Za-z0-9._-]*) die "'${name}' is not a valid project name" ;;
esac
case "$apps" in
'' | *[!0-9]*) die "APPS must be a non-negative integer, got '${apps}'" ;;
esac
if [ "$apps" -gt "$MAX_APPS" ]; then
	die "APPS must be at most ${MAX_APPS}, got ${apps}"
fi

dir=projects/${name}
[ -e "$dir" ] && die "${dir} already exists"
[ -d "$template" ] || die "missing ${template}"

napps=$((apps + 1))
slice=$((SLOT_CNT / napps))

cp -R "$template" "$dir"
rm -rf "${dir}/build"

tmp=$(mktemp -d)
trap 'rm -rf "$tmp"' EXIT
defs=${tmp}/defs
code=${tmp}/code
rows=${tmp}/rows
: >"$defs"
: >"$code"
: >"$rows"

app_base() {
	printf '0x%08x' "$((APP0_BASE + $1 * APP_SIZE))"
}

# ------------------------------------------------------------------ #
# One linker script and one main.c per extra application             #
# ------------------------------------------------------------------ #

i=1
while [ "$i" -le "$apps" ]; do
	base=$(app_base "$i")

	cat >"${dir}/app${i}.ld" <<EOF
MEMORY {
	RAM (rwx) : ORIGIN = ${base}, LENGTH = ${APP_SIZE}
}

__stack_size = 1024;
EOF

	mkdir -p "${dir}/app${i}"
	cat >"${dir}/app${i}/main.c" <<EOF
#include "../utils.h"
#include "altc/altio.h"
#include "s3k/s3k.h"

/* Granted by app0, already loaded into the PMP unit. */
#define MEM_CAP 0
#define UART_CAP 1
#define TIME_CAP 2

#define MEM_PMP_SLOT 0
#define UART_PMP_SLOT 1

static char trap_stack[1024];

int main(void)
{
	util_setup_trap(util_default_trap_handler, trap_stack,
			sizeof trap_stack);

	alt_printf("hello from pid %X at tick %X\n", s3k_get_pid(),
		   s3k_get_time());

	util_dump_caps();

	while (1)
		;
}
EOF

	# Defines and monitor calls inserted into app0/main.c.
	cat >>"$defs" <<EOF

/* app${i} */
#define APP${i}_PID ${i}
#define APP${i}_BASE ${base}
#define APP${i}_SIZE ${APP_SIZE}

/* Slots in app${i}'s capability table, and app${i}'s hardware PMP slots. */
#define APP${i}_MEM_CAP 0
#define APP${i}_UART_CAP 1
#define APP${i}_TIME_CAP 2
#define APP${i}_MEM_SLOT 0
#define APP${i}_UART_SLOT 1

/* Time slots on hart 0, one even share per application. */
#define APP${i}_TIME_BGN $(((i - 1) * slice))
#define APP${i}_TIME_END $((i * slice))
EOF

	cat >>"$code" <<EOF
	s3k_cidx_t app${i}_mem;

	err = util_grant_memory(APP${i}_PID, RAM_MEM, APP${i}_BASE, APP${i}_SIZE,
				S3K_MEM_RWX, APP${i}_MEM_CAP, APP${i}_MEM_SLOT,
				&app${i}_mem);
	util_check("app${i} memory", err);

	err = util_grant_device(APP${i}_PID, UART_MEM, UART0_BASE_ADDR, 0x8,
				S3K_MEM_RW, APP${i}_UART_CAP, APP${i}_UART_SLOT);
	util_check("app${i} uart", err);

	//err = util_grant_time(APP${i}_PID, HART0_TIME, APP${i}_TIME_BGN,
	//		      APP${i}_TIME_END, APP${i}_TIME_CAP);
	//util_check("app${i} time", err);
	//util_check("app${i} start", util_start(APP${i}_PID, APP${i}_BASE));

EOF

	cat >>"$rows" <<EOF
| \`app${i}.ld\` | Physical region for \`app${i}\`. Must not overlap another application. |
| \`app${i}/main.c\` | Entry point for \`app${i}\`. Runs once app0 grants it time and resumes it. |
EOF

	i=$((i + 1))
done

# ------------------------------------------------------------------ #
# app0: defines after the UART slot, monitor calls before the dump    #
# ------------------------------------------------------------------ #

if [ "$apps" -gt 0 ]; then
	awk -v defs="$defs" -v code="$code" '
	{
		if ($0 == "\tutil_dump_caps();" && !c) {
			while ((getline line < code) > 0)
				print line
			c = 1
		}
		print
		if ($0 == "#define UART_PMP_SLOT 1" && !d) {
			while ((getline line < defs) > 0)
				print line
			d = 1
		}
	}
	END { if (!d || !c) exit 1 }
	' "${dir}/app0/main.c" >"${tmp}/main.c" ||
		die "${template}/app0/main.c lost an anchor this script edits:
  '#define UART_PMP_SLOT 1' and '\tutil_dump_caps();'"
	mv "${tmp}/main.c" "${dir}/app0/main.c"
fi

# ------------------------------------------------------------------ #
# Makefile, s3k_conf.h, README                                       #
# ------------------------------------------------------------------ #

app_list=app0
i=1
while [ "$i" -le "$apps" ]; do
	app_list="${app_list} app${i}"
	i=$((i + 1))
done

grep -q '^APPS=app0$' "${dir}/Makefile" ||
	die "${template}/Makefile has no 'APPS=app0' line to update"
sed -i "s|^APPS=app0\$|APPS=${app_list}|" "${dir}/Makefile"

grep -q '^#define S3K_PROC_CNT ' "${dir}/s3k_conf.h" ||
	die "${template}/s3k_conf.h has no S3K_PROC_CNT to update"
if [ "$napps" -gt 2 ]; then
	sed -i "s|^#define S3K_PROC_CNT .*\$|#define S3K_PROC_CNT ${napps}|" \
		"${dir}/s3k_conf.h"
fi

if [ "$apps" -gt 0 ]; then
	awk -v rows="$rows" '
	{
		print
		if ($0 ~ /^\| `app0\/main\.c` \|/ && !r) {
			while ((getline line < rows) > 0)
				print line
			r = 1
		}
	}
	END { if (!r) exit 1 }
	' "${dir}/README.md" >"${tmp}/README.md" ||
		die "${template}/README.md has no app0/main.c table row"
	mv "${tmp}/README.md" "${dir}/README.md"

	sed -i "s|^## Adding a second process\$|## The other applications|" \
		"${dir}/README.md"
	awk -v apps="$apps" '
	{
		print
		if ($0 == "## The other applications" && !n) {
			print ""
			print "`make new APPS=" apps "` already did all of this:"
			print "the linker scripts, the `main.c` files, the `APPS` list,"
			print "`S3K_PROC_CNT`, and the grants in `app0/main.c`. It differs"
			print "from the steps below in one place: it carves a slice of"
			print "`HART0_TIME` with `util_grant_time` rather than moving a whole"
			print "hart, so the generated project also runs on single-hart"
			print "`qemu_virt`. The steps are kept because they still apply to an"
			print "application added by hand."
			n = 1
		}
	}
	' "${dir}/README.md" >"${tmp}/README2.md"
	mv "${tmp}/README2.md" "${dir}/README.md"
fi

# ------------------------------------------------------------------ #
# Summary                                                            #
# ------------------------------------------------------------------ #

echo "created ${dir} with ${napps} application(s): ${app_list}"
echo
echo "  build   make -C ${dir}"
echo "  run     make -C ${dir} qemu        (exit: C-a then x)"
echo "  debug   make -C ${dir} qemu-gdb    (then: gdb)"
echo
if [ "$apps" -gt 0 ]; then
	echo "app0 grants each of the others memory and UART. Uncomment the"
	echo "time and start calls in ${dir}/app0/main.c to run them."
	echo
fi
echo "see ${dir}/README.md"
