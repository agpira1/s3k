/* Addresses must match interface.h. */

MEMORY {
	RAM (rwx) : ORIGIN = 0x80030000, LENGTH = 0x7000
	TRAP (rwx) : ORIGIN = 0x80037000, LENGTH = 0x1000
}

/* Stack is outside the image, mapped on demand by the monitor. */
__stack_size = 0;

/* After .stack so _start's bss zeroing skips .trap. */
SECTIONS {
	/* Orphan .rodata would land in TRAP. */
	.rodata : { *(.rodata .rodata.* .srodata .srodata.*) } > RAM

	.trap : {
		KEEP(*(.trap.text))
		. = 0x1000;
	} > TRAP

	__stack_pointer = 0x80040000;
	_end = ADDR(.bss) + SIZEOF(.bss); /* empty .stack drops its _end */
} INSERT AFTER .stack;
