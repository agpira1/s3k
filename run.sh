#!/usr/bin/env bash
# Build the development image and run one S3K project under QEMU inside a
# throwaway container.
#
#   ./run.sh projects/hello
#   ./run.sh hello
#   ./run.sh hello qemu_virt4
#   ./run.sh canaries --tcp          # UART on localhost:4444 instead of here
#   ./run.sh canaries --tcp 5555
#
# Exit QEMU with Ctrl-a, then x.

set -euo pipefail

prog=${0##*/}

usage() {
	cat >&2 <<EOF
usage: ${prog} <project> [platform] [--tcp [port]]

  <project>   project directory, or just its name:
                ./${prog} projects/hello
                ./${prog} hello
  [platform]  override the project's default PLATFORM,
              e.g. qemu_virt or qemu_virt4
  --tcp [n]   put the guest UART on TCP port n (default 4444) and publish
              it, instead of attaching it to this terminal. QEMU waits for
              a client, so connect with "nc localhost <n>" or pwntools'
              remote("127.0.0.1", <n>) and no boot output is lost.

environment:
  S3K_TARGET  make target to run instead of "qemu", e.g. qemu-gdb or size
  S3K_BUILD   set to 0 to skip the image rebuild

The image copies the sources in, so the image is rebuilt by default to pick up
host edits. Exit QEMU with Ctrl-a, then x.
EOF
	exit 1
}

tcp_port=""
args=()
while [ $# -gt 0 ]; do
	case $1 in
	--tcp)
		tcp_port=4444
		# An optional port may follow, but the platform argument may
		# follow just as well, so only digits are taken as the port.
		case ${2:-} in
		"" | *[!0-9]*) ;;
		*)
			tcp_port=$2
			shift
			;;
		esac
		;;
	--tcp=*) tcp_port=${1#--tcp=} ;;
	-h | --help) usage ;;
	*) args+=("$1") ;;
	esac
	shift
done
set -- "${args[@]+"${args[@]}"}"

case $tcp_port in
"" | *[0-9]) ;;
*)
	echo "${prog}: --tcp takes a port number, got '${tcp_port}'" >&2
	exit 1
	;;
esac

if [ $# -lt 1 ] || [ $# -gt 2 ]; then
	usage
fi

root=$(cd -- "$(dirname -- "$0")" && pwd)
cd "$root"

# Accept "projects/hello", "projects/hello/", or "hello".
project=${1%/}
if [ ! -d "$project" ]; then
	project="projects/${project#projects/}"
fi

if [ ! -f "$project/Makefile" ]; then
	echo "${prog}: no project at '$1' (looked for $project/Makefile)" >&2
	echo "available projects:" >&2
	for d in projects/*/; do
		if [ -f "${d}Makefile" ]; then
			echo "  ${d#projects/}" >&2
		fi
	done
	exit 1
fi

platform=${2:-}
target=${S3K_TARGET:-qemu}

if [ ! -f compose.yaml ] && [ ! -f docker-compose.yaml ]; then
	echo "${prog}: no Compose file in ${root}" >&2
	exit 1
fi

if [ "${S3K_BUILD:-1}" != 0 ]; then
	echo "==> docker compose build" >&2
	docker compose build --quiet s3k
fi

cmd=(make -C "$project" "$target")
if [ -n "$platform" ]; then
	cmd+=("PLATFORM=$platform")
fi

# The port has to be published as well as passed in, or the socket QEMU binds
# inside the container is unreachable from the host.
run_opts=()
if [ -n "$tcp_port" ]; then
	run_opts+=(-p "${tcp_port}:${tcp_port}" -e "S3K_SERIAL_TCP=${tcp_port}")
fi

echo "==> ${cmd[*]}" >&2
if [ -n "$tcp_port" ]; then
	echo "==> UART on tcp 127.0.0.1:${tcp_port}, QEMU waits for a client" >&2
	echo "==> connect with: nc localhost ${tcp_port}" >&2
elif [ "$target" = qemu ]; then
	echo "==> exit QEMU with Ctrl-a, then x" >&2
fi

exec docker compose run --rm "${run_opts[@]+"${run_opts[@]}"}" s3k "${cmd[@]}"
