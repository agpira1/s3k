FROM docker.io/hakarlsson/riscv-picolibc

# The reference image is Ubuntu 24.04 and already provides everything the build
# needs: bash, make, git, the riscv64-unknown-elf toolchain,
# riscv64-unknown-elf-gdb, and qemu-system-riscv64.

WORKDIR /s3k

# Copy the kernel, common libraries, projects, and scripts into the image.
# See .dockerignore for what is left out of the build context.
COPY . /s3k

CMD ["bash"]
