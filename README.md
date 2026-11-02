# Compact PS2 exception display (experimental)

Freestanding PS2 EE proof-of-concept rewrite of the Code Designer exception display.

## IMPORTANT limitations

- **Not yet compiled or tested on PS2 hardware. Do not deploy as a drop-in replacement.**
- Linked for `0x0008A000`, the address in the supplied `Exception Display.cds`. Rebuild at your actual payload address.
- Entry point is at the start of the binary (`exception_entry`).
- Installer/vector patching is deliberately excluded: keep your per-game installation path.
- `exception_state` and emergency stack are in `.bss` and are **not included in the raw binary**. The loader must reserve and zero this memory, or the linker layout must be changed to include it.
- This captures **low 32 bits** of EE GPRs, as the original does, not the full 128-bit values.
- The current GS initialization is a minimal *prototype*. It inherits video timing, assumes a 640x448 framebuffer and does not yet clear VRAM or establish a full standalone display mode. Needs testing and correction against the original Pixel 2.0 routines.
- Exception entry temporarily uses the interrupted stack before switching to the emergency stack, like the supplied original. It may fail for invalid/unmapped SP exceptions.
- Cache and DMA setup require hardware verification.
- The glyph data is 95 printable ASCII characters, 11 bytes each, extracted from the supplied TinyFont.

Build with a PS2 EE GCC toolchain:

```sh
make
```

Check final size using `ls -l exception.bin`. Do not infer the raw payload size from the C source.
