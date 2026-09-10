#!/usr/bin/env bash
RED='\033[0;31m'
YELLOW='\033[0;33m'
NOCOLOR='\033[0m'

warning () {
	printf ${RED}"Warning: $1${NOCOLOR}\n"
}

info () {
	printf ${YELLOW}"$1${NOCOLOR}\n"
}

case ${PLATFORM} in
	qemu_virt)
		QEMU_MACHINE=virt
		QEMU_SMP=1
		QEMU_OPTIONS="-icount 3 $@"
		warning "Platform qemu_virt uses hart 0 only. If you want multiple cores, use qemu_virt4."
		;;
	qemu_virt4)
		QEMU_MACHINE=virt
		QEMU_SMP=4
		QEMU_OPTIONS="$@"
		;;
	sifive_unleashed)
		QEMU_MACHINE=sifive_u
		QEMU_SMP=5
		QEMU_OPTIONS="$@"
		warning "Platform sifive_unleashed uses harts 1, 2, 3, and 4. Hart 0 is present but not unused."
		;;
	sifive_unleashed4)
		QEMU_MACHINE=sifive_u
		QEMU_SMP=5
		QEMU_OPTIONS="$@"
		warning "Platform sifive_unleashed4 uses harts 1.  Harts 0, 2, 3, 4 are present but not unused."
		;;
	*)
		warning "Unknown platform ${PLATFORM}!"
		exit 1
esac


info "platform: ${PLATFORM}"
info "elfs: ${ELFS}"
info "machine: ${QEMU_MACHINE}"
info "smp: ${QEMU_SMP}"
info "extra options: ${QEMU_OPTIONS}"
info "Exit qemu with C-a+x"
info "Starting qemu ...\n"

LOADER=""
for elf in $ELFS; do
	LOADER+="-device loader,file=$elf "
done

# S3K_SERIAL_TCP puts the guest's UART on a TCP socket instead of this
# terminal, so a script can drive it. Its value is [host:]port; the host
# defaults to 0.0.0.0 so the port is reachable from outside a container.
# QEMU waits for a client before starting, so no boot output is lost.
if [ -n "${S3K_SERIAL_TCP:-}" ]; then
	case ${S3K_SERIAL_TCP} in
		*:*) SERIAL_ADDR=${S3K_SERIAL_TCP} ;;
		*)   SERIAL_ADDR=0.0.0.0:${S3K_SERIAL_TCP} ;;
	esac
	CONSOLE="-display none -serial tcp:${SERIAL_ADDR},server=on,wait=on"
	info "serial: tcp ${SERIAL_ADDR} (waiting for a client to connect)"
else
	CONSOLE="-nographic"
fi

qemu-system-riscv64 \
	-M $QEMU_MACHINE \
	-smp $QEMU_SMP \
	-m 128M $CONSOLE \
	-bios none \
	$LOADER \
	$QEMU_OPTIONS
