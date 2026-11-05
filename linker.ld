ENTRY(exceptionStart)
SECTIONS {
	. = 0x000c8000;
	.text : { KEEP(*(.text.entry)) *(.text*) }
	.rodata : { *(.rodata*) }
	.data : { *(.data*) *(.sdata*) }
	. = ALIGN(16);
	__bss_start = .;
	.bss (NOLOAD) : { *(.bss*) *(.sbss*) *(COMMON) }
	. = ALIGN(16);
	exceptionMemoryEnd = .;
	/DISCARD/ : { *(.reginfo) *(.MIPS.abiflags) *(.pdr) *(.comment) *(.note*) *(.eh_frame*) }
	ASSERT(exceptionStart == 0x000c8000, "Horizon installer entry moved")
	ASSERT(exceptionMemoryEnd <= 0x000cb000, "Exception payload exceeds Horizon reservation")
}
