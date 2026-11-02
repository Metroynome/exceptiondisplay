.set noreorder
.set noat
.text
.align 4
.globl exception_entry
.ent exception_entry
exception_entry:
    addiu $sp, $sp, -16
    sq    $k0, 0($sp)
    lui   $k0, %hi(exception_state)
    addiu $k0, $k0, %lo(exception_state)
    sw    $at, 4($k0)
    sw    $v0, 8($k0)
    sw    $v1, 12($k0)
    sw    $a0, 16($k0)
    sw    $a1, 20($k0)
    sw    $a2, 24($k0)
    sw    $a3, 28($k0)
    sw    $t0, 32($k0)
    sw    $t1, 36($k0)
    sw    $t2, 40($k0)
    sw    $t3, 44($k0)
    sw $12, 48($26)
    sw $13, 52($26)
    sw $14, 56($26)
    sw $15, 60($26)
    sw    $s0, 64($k0)
    sw    $s1, 68($k0)
    sw    $s2, 72($k0)
    sw    $s3, 76($k0)
    sw    $s4, 80($k0)
    sw    $s5, 84($k0)
    sw    $s6, 88($k0)
    sw    $s7, 92($k0)
    sw    $t8, 96($k0)
    sw    $t9, 100($k0)
    sw    $k1, 108($k0)
    sw    $gp, 112($k0)
    sw    $fp, 120($k0)
    sw    $ra, 124($k0)
    sw    $zero, 0($k0)
    lq    $k1, 0($sp)
    sw    $k1, 104($k0)
    addiu $k1, $sp, 16
    sw    $k1, 116($k0)
    mfc0  $k1, $13
    sw    $k1, 128($k0)
    mfc0  $k1, $14
    sw    $k1, 132($k0)
    mfc0  $k1, $8
    sw    $k1, 136($k0)
    mfc0  $k1, $12
    sw    $k1, 140($k0)
    lui   $sp, %hi(exception_stack_top)
    lw    $sp, %lo(exception_stack_top)($sp)
    lui   $k1, %hi(exception_screen)
    addiu $k1, $k1, %lo(exception_screen)
    mtc0  $k1, $14
    sync
    eret
    nop
.end exception_entry
