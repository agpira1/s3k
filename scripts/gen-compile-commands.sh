#!/usr/bin/env bash
# Generate compile_commands.json for one project, so clangd and the VS Code
# C/C++ extension see the same include paths, defines, and forced includes the
# build actually uses.
#
#   ./scripts/gen-compile-commands.sh                 # defaults to projects/hello
#   ./scripts/gen-compile-commands.sh tutorial.03.mem-cap
#   ./scripts/gen-compile-commands.sh hello qemu_virt
#
# The flags are taken from `make -n`, so they cannot drift from the Makefiles.
# No compiler needs to be installed: make only prints the commands.
#
# Only one project can be described at a time, because each project
# force-includes its own s3k_conf.h and the kernel is compiled per project.

set -euo pipefail

prog=${0##*/}
root=$(cd -- "$(dirname -- "$0")/.." && pwd)
cd "$root"

usage() {
	cat >&2 <<EOF
usage: ${prog} [project] [platform]

  [project]   project directory or name (default: projects/hello)
  [platform]  override PLATFORM, e.g. qemu_virt or qemu_virt4

Writes compile_commands.json in the repository root.
EOF
	exit 1
}

if [ $# -gt 2 ]; then
	usage
fi

project=${1:-projects/hello}
project=${project%/}
if [ ! -d "$project" ]; then
	project="projects/${project#projects/}"
fi
if [ ! -f "$project/Makefile" ]; then
	echo "${prog}: no project at '${1:-projects/hello}'" >&2
	exit 1
fi

# Platform: explicit argument, else the project Makefile's own default.
platform=${2:-}
if [ -z "$platform" ]; then
	platform=$(sed -n 's/^[[:space:]]*export[[:space:]]*PLATFORM[[:space:]]*?*=[[:space:]]*//p' \
		"$project/Makefile" | head -1)
fi
if [ -z "$platform" ]; then
	echo "${prog}: could not determine PLATFORM for $project" >&2
	exit 1
fi

build="$root/$project/build/$platform"
conf="$root/$project/s3k_conf.h"

# Applications listed by the project Makefile.
apps=$(sed -n 's/^APPS[[:space:]]*=[[:space:]]*//p' "$project/Makefile" | head -1)

raw=$(mktemp)
trap 'rm -f "$raw"' EXIT

# Stage 1: common and the kernel. A dry run from the project directory
# recurses into both with the project's own configuration.
make -Bkn -C "$project" PLATFORM="$platform" >>"$raw" 2>/dev/null || true

# Stage 2: the applications. Their ELF rule depends on a startup object built
# by a different makefile, which a forced dry run cannot resolve, so ask for
# each object file directly instead of for the whole program.
for app in $apps; do
	objs=""
	for src in "$project/$app"/*.c "$project/$app"/*.S; do
		[ -e "$src" ] || continue
		base=${src##*/}
		objs="$objs $build/$app/${base%.*}.o"
	done
	[ -n "$objs" ] || continue
	# shellcheck disable=SC2086
	PLATFORM="$platform" ROOT="$root" BUILD="$build" S3K_CONF_H="$conf" \
		make -Bkn -C "$project" -f ../build.mk PROGRAM="$app" $objs \
		>>"$raw" 2>/dev/null || true
done

awk '
function esc(s) {
	gsub(/\\/, "\\\\", s)
	gsub(/"/, "\\\"", s)
	return s
}
/: Entering directory / {
	line = $0
	sub(/^.*: Entering directory ./, "", line)
	sub(/.$/, "", line)
	dir[++depth] = line
	next
}
/: Leaving directory / {
	if (depth > 0) depth--
	next
}
# Compile lines have the shape: <compiler> -o <object> <source> ... -c ...
$2 == "-o" && $0 ~ /-c( |$)/ && $1 ~ /gcc$|cc$|clang$/ {
	src = $4
	if (src !~ /\.(c|S|s)$/) next
	cwd = (depth > 0) ? dir[depth] : ENVIRON["PWD"]
	file = (src ~ /^\//) ? src : cwd "/" src
	if (file in seen) next
	seen[file] = 1
	entries[++n] = "  {\n    \"directory\": \"" esc(cwd) "\",\n" \
		"    \"file\": \"" esc(file) "\",\n" \
		"    \"command\": \"" esc($0) "\"\n  }"
}
END {
	print "["
	for (i = 1; i <= n; i++)
		printf "%s%s\n", entries[i], (i < n ? "," : "")
	print "]"
	printf "%s: %d entries\n", "gen-compile-commands", n > "/dev/stderr"
}
' "$raw" >compile_commands.json

echo "${prog}: wrote $root/compile_commands.json for $project ($platform)" >&2
