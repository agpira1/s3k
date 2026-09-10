.POSIX:

export PLATFORM   ?=qemu_virt
export ROOT       :=${abspath ../..}
export BUILD      :=${abspath build/${PLATFORM}}
export S3K_CONF_H :=${abspath s3k_conf.h}

include ${ROOT}/tools.mk

APPS=app0 monitor vuln

SHARED_SRCS=utils.c

ELFS:=${patsubst %, ${BUILD}/%.elf, kernel ${APPS}}

all: kernel ${APPS}

clean:
	rm -rf ${BUILD}

common:
	@${MAKE} -C ${ROOT}/common

kernel: common
	@${MAKE} -C ${ROOT}/kernel

${APPS}: common
	@${MAKE} -f ../build.mk PROGRAM=$@ SHARED_SRCS="${SHARED_SRCS}"

qemu: kernel ${APPS}
	@ELFS="${ELFS}" ${ROOT}/scripts/qemu.sh

# Serial on a TCP socket instead of this terminal, for driving the guest from
# a script (pwntools, netcat). QEMU waits for a client before it starts.
TCP_PORT ?=4444

qemu-tcp: kernel ${APPS}
	@S3K_SERIAL_TCP=${TCP_PORT} ELFS="${ELFS}" ${ROOT}/scripts/qemu.sh

qemu-gdb: kernel ${APPS}
	@ELFS="${ELFS}" ${ROOT}/scripts/qemu.sh -gdb tcp::3333 -S

gdb: kernel ${APPS}
	@ELFS="${ELFS}" ${ROOT}/scripts/gdb.sh

gdb-openocd: kernel ${APPS}
	@ELFS="${ELFS}" ${ROOT}/scripts/gdb-openocd.sh

size: ${ELFS}
	${SIZE} ${ELFS}

.PHONY: all clean size qemu qemu-tcp qemu-gdb gdb kernel common ${APPS}
