#ifndef COMPACT_EXCEPTION_H
#define COMPACT_EXCEPTION_H
#include <stdint.h>

typedef uint8_t u8;
typedef uint32_t u32;
typedef uint64_t u64;
#include "exception_protocol.h"

typedef struct exceptionState {
	/* 0x00 */ u32 gpr[32];
	/* 0x80 */ u32 cause;
	/* 0x84 */ u32 epc;
	/* 0x88 */ u32 badvaddr;
	/* 0x8c */ u32 status;
} exceptionState_t;

extern volatile exceptionState_t exceptionState;
extern volatile exceptionContext_t exceptionContext;
void exceptionScreen(void) __attribute__((noreturn));
void exceptionEntry(void);
int exceptionInstall(void);
#endif
