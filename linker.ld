ENTRY(exception_entry)
SECTIONS {
 . = 0x0008A000;
 .text : { *(.text*) }
 .rodata : { *(.rodata*) }
 .data : { *(.data*) }
 .bss (NOLOAD) : { *(.bss*) *(COMMON) }
}
