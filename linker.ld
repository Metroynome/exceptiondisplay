ENTRY(exception_entry)
SECTIONS {
 . = 0x000c0000;
 .text : { *(.text*) }
 .rodata : { *(.rodata*) }
 .data : { *(.data*) }
 .bss (NOLOAD) : { *(.bss*) *(COMMON) }
}
