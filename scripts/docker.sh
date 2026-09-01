#!/bin/sh

set -eu

WORKSPACE="$(pwd -P)"
SCRIPT_DIR="$(CDPATH= cd -- "$(dirname "$0")" && pwd -P)"
REPO_ROOT="$(CDPATH= cd -- "$SCRIPT_DIR/.." && pwd -P)"
IMAGE_NAME="${S3K_DOCKER_IMAGE:-s3k/riscv-picolibc}"
DOCKER_PLATFORM="${S3K_DOCKER_PLATFORM:-linux/amd64}"

docker build --platform "$DOCKER_PLATFORM" -t "$IMAGE_NAME" -f "$REPO_ROOT/.devcontainer/Dockerfile" "$REPO_ROOT/.devcontainer"

docker run --platform "$DOCKER_PLATFORM" -it --rm \
	-p "5555:5555" \
	-v "$WORKSPACE:/workspace" \
	-w /workspace \
	"$IMAGE_NAME"
