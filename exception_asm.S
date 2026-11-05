.set noreorder
.set noat
.section .text.entry,"ax",@progbits
.align 4
.globl exceptionStart
exceptionStart:
	j exceptionInstall
	nop
	.word 0x45584332
	.word 1
	.word exceptionContext
	.word exceptionEntry
	.word exceptionMemoryEnd
	.word 0

.text
.align 4
.globl exceptionEntry
.ent exceptionEntry
exceptionEntry:
	# Fatal capture must not use the interrupted stack. HI is no longer needed.
	mthi $27
	lui $27, %hi(exceptionState)
	addiu $27, $27, %lo(exceptionState)
	sw $26, 104($27)
	mfhi $26
	sw $26, 108($27)
	sw $0, 0($27)
	sw $1, 4($27)
	sw $2, 8($27)
	sw $3, 12($27)
	sw $4, 16($27)
	sw $5, 20($27)
	sw $6, 24($27)
	sw $7, 28($27)
	sw $8, 32($27)
	sw $9, 36($27)
	sw $10, 40($27)
	sw $11, 44($27)
	sw $12, 48($27)
	sw $13, 52($27)
	sw $14, 56($27)
	sw $15, 60($27)
	sw $16, 64($27)
	sw $17, 68($27)
	sw $18, 72($27)
	sw $19, 76($27)
	sw $20, 80($27)
	sw $21, 84($27)
	sw $22, 88($27)
	sw $23, 92($27)
	sw $24, 96($27)
	sw $25, 100($27)
	sw $28, 112($27)
	sw $29, 116($27)
	sw $30, 120($27)
	sw $31, 124($27)
	mfc0 $26, $13
	sw $26, 128($27)
	mfc0 $26, $14
	sw $26, 132($27)
	mfc0 $26, $8
	sw $26, 136($27)
	mfc0 $26, $12
	sw $26, 140($27)
	lui $29, %hi(exceptionStackTop)
	addiu $29, $29, %lo(exceptionStackTop)
	lui $26, %hi(exceptionScreen)
	addiu $26, $26, %lo(exceptionScreen)
	mtc0 $26, $14
	sync.p
	eret
	nop
.end exceptionEntry

# PS2SDK syscall ABI: v1 selects the kernel service; a0/a1 are arguments.
.macro kernelCall name, number
.globl \name
.ent \name
\name:
	addiu $3, $0, \number
	syscall
	jr $31
	nop
.end \name
.endm
kernelCall exceptionSetTlb, 0x0d
kernelCall exceptionSetCommon, 0x0e
kernelCall exceptionFlushCache, 0x64

.section .bss,"aw",@nobits
.align 4
exceptionStack:
	.space 2048
.globl exceptionStackTop
exceptionStackTop:
