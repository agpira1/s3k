#!/usr/bin/env bash
# Build the development image and run one S3K project under QEMU inside a
# throwaway container.
#
#   ./run.sh projects/hello
#   ./run.sh hello
#   ./run.sh hello qemu_virt4
#
# Exit QEMU with Ctrl-a, then x.

set -euo pipefail

prog=${0##*/}

usage() {
	cat >&2 <<EOF
usage: ${prog} <project> [platform]

  <project>   project directory, or just its name:
                ./${prog} projects/hello
                ./${prog} hello
  [platform]  override the project's default PLATFORM,
              e.g. qemu_virt or qemu_virt4

environment:
  S3K_TARGET  make target to run instead of "qemu", e.g. qemu-gdb or size
  S3K_BUILD   set to 0 to skip the image rebuild

The image copies the sources in, so the image is rebuilt by default to pick up
host edits. Exit QEMU with Ctrl-a, then x.
EOF
	exit 1
}

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

echo "==> ${cmd[*]}" >&2
if [ "$target" = qemu ]; then
	echo "==> exit QEMU with Ctrl-a, then x" >&2
fi

exec docker compose run --rm s3k "${cmd[@]}"
