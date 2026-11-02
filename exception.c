#include "exception.h"
#include "font.h"

volatile ExceptionState exception_state;
static unsigned char emergency_stack[2048] __attribute__((aligned(16)));
void *const exception_stack_top = emergency_stack + sizeof(emergency_stack);

/* GS privileged registers, accessed through EE MMIO. */
#define MMIO64(a) (*(volatile uint64_t *)(uintptr_t)(a))
#define GS_PMODE   0x12000000u
#define GS_SMODE2  0x12000020u
#define GS_DISPFB1 0x12000070u
#define GS_DISPLAY1 0x12000080u
#define GS_BGCOLOR 0x120000E0u

/* GIF channel DMA registers. */
#define D2_CHCR 0x1000A000u
#define D2_MADR 0x1000A010u
#define D2_QWC  0x1000A020u

/* GS GIF A+D register addresses */
#define GS_PRIM 0x00u
#define GS_RGBAQ 0x01u
#define GS_XYZ2 0x05u
#define GS_XYOFFSET_1 0x18u
#define GS_SCISSOR_1 0x40u
#define GS_FRAME_1 0x4Cu
#define GS_ZBUF_1 0x4Eu
#define GS_TEST_1 0x47u

typedef struct { uint64_t lo, hi; } Qword;
static Qword packet[512] __attribute__((aligned(16)));
static unsigned count;

/* One GIFtag, then A+D writes. GIFtag NLOOP is filled at flush. */
static void flush(void) {
    if (!count) return;
    packet[0].lo = (uint64_t)count | (1ull << 15) | (1ull << 60);
    packet[0].hi = 0xE; /* A+D */
    /* Writeback cache before GIF DMA. */
    for (unsigned i = 0; i <= count; i++)
        __asm__ volatile("cache 0x15, 0(%0)" :: "r"(&packet[i]) : "memory");
    while (MMIO64(D2_CHCR) & 0x100u) {}
    MMIO64(D2_MADR) = (uint32_t)(uintptr_t)packet;
    MMIO64(D2_QWC) = count + 1;
    MMIO64(D2_CHCR) = 0x101u;
    while (MMIO64(D2_CHCR) & 0x100u) {}
    count = 0;
}
static void regwrite(unsigned reg, uint64_t value) {
    if (count == 510) flush();
    packet[++count].lo = value;
    packet[count].hi = reg;
}
static void rect(int x, int y, int w, int h, uint32_t color) {
    /* Coordinates use 12.4 fixed point with XYOFFSET=0. */
    regwrite(GS_PRIM, 6); /* sprite, context 1 */
    regwrite(GS_RGBAQ, (uint64_t)color | (1ull << 32));
    regwrite(GS_XYZ2, ((uint64_t)(y << 4) << 32) | (unsigned)(x << 4));
    regwrite(GS_XYZ2, ((uint64_t)((y+h) << 4) << 32) | (unsigned)((x+w) << 4));
}
static void draw_char(int x, int y, char ch) {
    if ((unsigned char)ch < 32 || (unsigned char)ch > 126) ch = '?';
    const unsigned char *rows = font[(unsigned char)ch - 32];
    for (int row = 0; row < 11; row++) {
        unsigned bits = rows[row];
        for (int col = 0; col < 8; col++)
            if (bits & (1u << col)) rect(x + col * 2, y + row * 2, 2, 2, 0x80FFFFFFu);
    }
}
static void draw_text(int x, int y, const char *s) {
    int left = x;
    while (*s) {
        char c = *s++;
        if (c == '\n') { x = left; y += 26; continue; }
        draw_char(x, y, c);
        x += 18;
    }
}
static void hex8(int x, int y, uint32_t v) {
    static const char digits[] = "0123456789abcdef";
    for (int n = 7; n >= 0; n--) {
        draw_char(x + (7-n)*18, y, digits[(v >> (n*4)) & 15]);
    }
}
static void label_hex(int x, int y, const char *name, uint32_t v) {
    draw_text(x, y, name);
    hex8(x + 12*18, y, v);
}
static void init_gs(void) {
    /* 640x448, 32-bit framebuffer at VRAM base 0.
       Display timing is inherited from the game's video mode. */
    MMIO64(GS_PMODE) = 0;
    MMIO64(GS_DISPFB1) = (10ull << 9); /* FBW = 640 / 64 */
    MMIO64(GS_DISPLAY1) = (uint64_t)0x27F | ((uint64_t)0x1BF << 44);
    MMIO64(GS_BGCOLOR) = 0;
    MMIO64(GS_PMODE) = 1;
    regwrite(GS_FRAME_1, (10ull << 16)); /* 640px, PSMCT32 */
    regwrite(GS_ZBUF_1, (1ull << 32));   /* ZMSK */
    regwrite(GS_TEST_1, 0);
    regwrite(GS_XYOFFSET_1, 0);
    regwrite(GS_SCISSOR_1, 639ull << 16 | 447ull << 48);
    flush();
}
void exception_screen(void) {
    static const char names[32][3] = {
        "zr","at","v0","v1","a0","a1","a2","a3",
        "t0","t1","t2","t3","t4","t5","t6","t7",
        "s0","s1","s2","s3","s4","s5","s6","s7",
        "t8","t9","gp","fp","sp","k0","k1","ra"
    };
    init_gs();
    /* Drawing once is enough for a stopped game; no game frame loop. */
    draw_text(12, 10, "A fatal error has occurred.");
    label_hex(12, 42, "Cause", exception_state.cause);
    label_hex(320, 42, "EPC", exception_state.epc);
    label_hex(12, 68, "BadVAddr", exception_state.badvaddr);
    label_hex(320, 68, "Status", exception_state.status);
    draw_text(12, 96, "Register dump");
    for (int i = 0; i < 32; i++) {
        int x = (i < 16) ? 12 : 320;
        int y = 126 + (i & 15) * 19;
        draw_text(x, y, names[i]);
        hex8(x + 55, y, exception_state.gpr[i]);
    }
    flush();
    for (;;) __asm__ volatile("nop");
}
