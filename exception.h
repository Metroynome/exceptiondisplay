#ifndef COMPACT_EXCEPTION_H
#define COMPACT_EXCEPTION_H
#include <stdint.h>
typedef struct {
    uint32_t gpr[32];  /* low 32 bits, same as the original display */
    uint32_t cause, epc, badvaddr, status;
} ExceptionState;
extern volatile ExceptionState exception_state;
void exception_screen(void) __attribute__((noreturn));
void exception_entry(void);
#endif
