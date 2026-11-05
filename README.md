# Horizon exception display

Build with `make` using the native PS2 EE toolchain. `exception.bin` loads at
`0x000c8000`; call that address once to clear BSS and install the fatal vectors.
The linker requires the complete workspace to end at or before `0x000cb000`.
Do not clear BSS separately or enter the handler directly to install it.

`exception_protocol.h` describes the 32-byte payload header and 44-byte mutable
context on the 32-bit EE ABI. Horizon's `runExceptionHandler` recognizes its
magic/version, calls the installer, and updates the region, map and progressive
scan flag. Keep the matching header in Horizon synchronized. The old payload
remains supported through Horizon's legacy path.

Copy `exception.bin` to `horizon-uya-patch/misc/exceptiondisplay.bin` and deploy
the matching rebuilt Horizon patch. Restart the local server to reload its
cached files, then cold boot the game. Do not replace this payload in a running
game after its vectors have been installed.

Fatal exceptions use a private stack and draw through GIF DMA without game
functions. Syscalls, interrupts and break exceptions retain their native handlers.
The display preserves CRT timing and uses Horizon's interlace/progressive window
values. GS XYZ2 packs X at bit 0 and Y at bit 16; EE cache writeback uses `0x18`.

Verified in NTSC-U PCSX2 with an installed-handler TEQ test: the register screen,
region/map text, cause `0x34`, and EPC `0x000caf00` appeared. Both Horizon regions
build; PAL and progressive scan still require runtime testing. A standalone trap
test does not establish successful online patch download or map transitions.

The background matches the legacy RGB (48, 48, 160). The footer at (86, 410)
shows the fatal-error message by default. Horizon supplies region/map text only
when a lobby or DME connection exists, and clears it otherwise. The footer uses
the legacy six-character region and twenty-character area budget. Both footer
branches were visually checked in NTSC-U PCSX2 using supplied/empty context;
actual online-to-offline transitions still need gameplay testing.
