#include "exception.h"
#include "font.h"

#define MMIO32(a) (*(volatile u32*)(uintptr_t)(a))
#define MMIO64(a) (*(volatile u64*)(uintptr_t)(a))
#define GS_PMODE (0x12000000)
#define GS_DISPFB1 (0x12000070)
#define GS_DISPLAY1 (0x12000080)
#define GS_DISPLAY2 (0x120000a0)
#define GS_BGCOLOR (0x120000e0)
#define GIF_CTRL (0x10003000)
#define D2_CHCR (0x1000a000)
#define D2_MADR (0x1000a010)
#define D2_QWC (0x1000a020)
#define PACKET_QWORDS (128)

typedef struct qword {
	/* 0x00 */ u64 lo;
	/* 0x08 */ u64 hi;
} qword_t;

extern u8 __bss_start[];
extern u8 exceptionMemoryEnd[];
extern void exceptionSetTlb(int number, void* handler);
extern void exceptionSetCommon(int number, void* handler);
extern void exceptionFlushCache(int operation);

volatile exceptionState_t exceptionState;
volatile exceptionContext_t exceptionContext;
static qword_t packet[PACKET_QWORDS] __attribute__((aligned(64)));
static u32 count;

/*
 * NAME : exceptionInstall
 * DESCRIPTION : Initialize the workspace and install the fatal exception vectors.
 * NOTES : Syscalls and interrupts retain their native handlers.
 * ARGS :
 * RETURN :
 * AUTHOR : Troy "Metroynome" Pruitt
 */
int exceptionInstall(void)
{
	volatile u8* next = __bss_start;
	while (next < exceptionMemoryEnd)
		*next++ = 0;

	/* Same fatal vector set as the original Horizon payload and PS2SDK eedebug. */
	int i;
	for (i = 1; i <= 3; ++i)
		exceptionSetTlb(i, exceptionEntry);
	for (i = 4; i <= 7; ++i)
		exceptionSetCommon(i, exceptionEntry);
	for (i = 10; i <= 13; ++i)
		exceptionSetCommon(i, exceptionEntry);
	exceptionFlushCache(0);
	exceptionFlushCache(2);
	return 1;
}

/*
 * NAME : flush
 * DESCRIPTION : Submit buffered GS writes through GIF DMA.
 * NOTES : 
 * ARGS :
 * RETURN :
 * AUTHOR : Troy "Metroynome" Pruitt
 */
static void flush(void)
{
	if (!count)
		return;
	packet[0].lo = (u64)count | ((u64)1 << 15) | ((u64)1 << 60);
	packet[0].hi = 0x0e;
	u32 i;
	for (i = 0; i <= count; i += 4)
		__asm__ volatile("cache 0x18, 0(%0)" :: "r"(&packet[i]) : "memory");
	__asm__ volatile("sync.l" ::: "memory");
	while (MMIO32(D2_CHCR) & 0x100) {}
	MMIO32(D2_MADR) = (u32)(uintptr_t)packet;
	MMIO32(D2_QWC) = count + 1;
	MMIO32(D2_CHCR) = 0x101;
	while (MMIO32(D2_CHCR) & 0x100) {}
	count = 0;
}

/*
 * NAME : regwrite
 * DESCRIPTION : Append one GS register write.
 * NOTES : 
 * ARGS :
 * RETURN :
 * AUTHOR : Troy "Metroynome" Pruitt
 */
static void regwrite(u32 reg, u64 value)
{
	if (count == PACKET_QWORDS - 2)
		flush();
	packet[++count].lo = value;
	packet[count].hi = reg;
}

/*
 * NAME : rect
 * DESCRIPTION : Draw an untextured rectangle.
 * NOTES : 
 * ARGS :
 * RETURN :
 * AUTHOR : Troy "Metroynome" Pruitt
 */
static void rect(int x, int y, int w, int h, u32 color)
{
	regwrite(0x00, 6);
	regwrite(0x01, (u64)color | ((u64)1 << 32));
	regwrite(0x05, ((u64)(y << 4) << 16) | (u32)(x << 4));
	regwrite(0x05, ((u64)((y + h) << 4) << 16) | (u32)((x + w) << 4));
}

/*
 * NAME : drawChar
 * DESCRIPTION : Draw a font glyph without overlapping adjacent register rows.
 * NOTES : 
 * ARGS :
 * RETURN :
 * AUTHOR : Troy "Metroynome" Pruitt
 */
static void drawChar(int x, int y, char ch)
{
	if ((u8)ch < 32 || (u8)ch > 126)
		ch = '?';
	const unsigned char* rows = font[(u8)ch - 32];
	int row, col;
	for (row = 0; row < 11; ++row) {
		u32 bits = rows[row];
		for (col = 0; col < 8; ++col) {
			if (bits & (1 << col))
				rect(x + col * 2, y + row, 2, 1, 0x80ffffff);
		}
	}
}

/*
 * NAME : drawText
 * DESCRIPTION : Draw a terminated string.
 * NOTES : 
 * ARGS :
 * RETURN :
 * AUTHOR : Troy "Metroynome" Pruitt
 */
static void drawText(int x, int y, const char* text)
{
	int left = x;
	while (*text) {
		char ch = *text++;
		if (ch == '\n') {
			x = left;
			y += 19;
			continue;
		}
		drawChar(x, y, ch);
		x += 18;
	}
}

/*
 * NAME : hex8
 * DESCRIPTION : Draw a full 32-bit hexadecimal value.
 * NOTES : 
 * ARGS :
 * RETURN :
 * AUTHOR : Troy "Metroynome" Pruitt
 */
static void hex8(int x, int y, u32 value)
{
	static const char digits[] = "0123456789abcdef";
	int n;
	for (n = 7; n >= 0; --n)
		drawChar(x + (7 - n) * 18, y, digits[(value >> (n * 4)) & 15]);
}

/*
 * NAME : labelHex
 * DESCRIPTION : Draw a CP0 register name and value.
 * NOTES : 
 * ARGS :
 * RETURN :
 * AUTHOR : Troy "Metroynome" Pruitt
 */
static void labelHex(int x, int y, const char* name, u32 value)
{
	drawText(x, y, name);
	hex8(x + 7 * 18, y, value);
}

/*
 * NAME : initGs
 * DESCRIPTION : Configure the fatal display using Horizon video-mode adjustments.
 * NOTES : 
 * ARGS :
 * RETURN :
 * AUTHOR : Troy "Metroynome" Pruitt
 */
static void initGs(void)
{
	/* Preserve CRT timing; only replace the display window and framebuffer. */
	MMIO32(D2_CHCR) = 0;
	MMIO32(GIF_CTRL) = 1;
	MMIO32(GIF_CTRL) = 0;
	MMIO64(GS_PMODE) = 0;
	MMIO64(GS_DISPFB1) = (u64)10 << 9;
	u64 display = ((u64)0x001bf9ff << 32) |
		(exceptionContext.progressive ? 0x0083210e : 0x01832278);
	MMIO64(GS_DISPLAY1) = display;
	MMIO64(GS_DISPLAY2) = display;
	MMIO64(GS_BGCOLOR) = 0x00a03030;
	MMIO64(GS_PMODE) = 0x8025; /* RC1, required CRTMD, fixed opaque alpha */
	regwrite(0x1a, 1); /* PRMODECONT */
	regwrite(0x46, 1); /* COLCLAMP */
	regwrite(0x45, 0); /* DTHE */
	regwrite(0x4c, (u64)10 << 16); /* FRAME_1: 640 pixels, PSMCT32 */
	regwrite(0x4e, (u64)1 << 32); /* ZBUF_1: masked */
	regwrite(0x47, 0x30000); /* TEST_1: enable Z test with ALWAYS, writes masked */
	regwrite(0x18, 0); /* XYOFFSET_1 */
	regwrite(0x40, ((u64)639 << 16) | ((u64)447 << 48));
	rect(0, 0, 640, 448, 0x80a03030);
	flush();
}

/*
 * NAME : exceptionScreen
 * DESCRIPTION : Display the captured registers and the latest region/map context.
 * NOTES : 
 * ARGS :
 * RETURN :
 * AUTHOR : Troy "Metroynome" Pruitt
 */
void exceptionScreen(void)
{
	static const char names[32][3] = {
		"zr", "at", "v0", "v1", "a0", "a1", "a2", "a3",
		"t0", "t1", "t2", "t3", "t4", "t5", "t6", "t7",
		"s0", "s1", "s2", "s3", "s4", "s5", "s6", "s7",
		"t8", "t9", "k0", "k1", "gp", "sp", "fp", "ra"
	};
	/* No game or SDK calls after a fatal exception. */
	__asm__ volatile("di\n\tsync.p" ::: "memory");
	count = 0;
	exceptionContext.region[7] = 0;
	exceptionContext.map[31] = 0;
	initGs();
	if (exceptionContext.map[0]) {
		/* Match the legacy footer's six region and twenty area characters. */
		exceptionContext.map[20] = 0;
		drawText(86, 410, (const char*)exceptionContext.region);
		drawText(194, 410, (const char*)exceptionContext.map);
	} else {
		drawText(86, 410, "A fatal error has occurred.");
	}
	labelHex(12, 50, "Cause", exceptionState.cause);
	labelHex(320, 50, "EPC", exceptionState.epc);
	labelHex(12, 69, "BadAddr", exceptionState.badvaddr);
	labelHex(320, 69, "Status", exceptionState.status);
	int i;
	for (i = 0; i < 32; ++i) {
		int x = i < 16 ? 12 : 320;
		int y = 102 + (i & 15) * 19;
		drawText(x, y, names[i]);
		hex8(x + 55, y, exceptionState.gpr[i]);
	}
	flush();
	for (;;)
		__asm__ volatile("nop");
}

