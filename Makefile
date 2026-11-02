EE_PREFIX ?= mips64r5900el-ps2-elf-
CC = $(EE_PREFIX)gcc
OBJCOPY = $(EE_PREFIX)objcopy
SIZE = $(EE_PREFIX)size
CFLAGS = -O2 -G0 -ffreestanding -fno-builtin -fno-pic -fno-pie -fno-stack-protector -fno-asynchronous-unwind-tables -mabi=n32
LDFLAGS = -nostdlib -nostartfiles -Wl,-T,linker.ld -Wl,-Map,exception.map
all: exception.bin
exception.elf: exception.c exception_asm.S exception.h font.h linker.ld
	$(CC) $(CFLAGS) $(LDFLAGS) exception_asm.S exception.c -o $@
exception.bin: exception.elf
	$(OBJCOPY) -O binary $< $@
	$(SIZE) $<

ifeq ($(OS),Windows_NT)
clean:
	-cmd /C "del /Q exception.elf exception.bin exception.map 2>NUL"
else
clean:
	rm -f exception.elf exception.bin exception.map
endif

.PHONY: all clean
