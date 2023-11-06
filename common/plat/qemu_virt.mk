export ARCH=rv64imac_zicsr
export ABI=lp64
export CMODEL=medany
export QEMU_MACHINE=virt
export QEMU_SMP=1
export QEMU_OPTIONS=-icount 3
export COMMON_INC:=${ROOT}/common/inc
export COMMON_LIB:=${ROOT}/common/build/${PLATFORM}
export STARTFILES:=${ROOT}/common/build/${PLATFORM}/start
PLAT_SRCS=src/drivers/uart/ns16550a.c src/drivers/time.c
$(info Platform qemu_virt uses hart 0 only. If you want multiple cores, use qemu_virt4.)
