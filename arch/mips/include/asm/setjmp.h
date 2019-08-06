/*
 * Copyright (c) 2016 Imagination Technologies
 *
 * SPDX-License-Identifier:	GPL-2.0+
 */
#ifndef __MIPS_ASM_SETJMP_H__
#define __MIPS_ASM_SETJMP_H__

/**
 * struct jmp_buf - Holds callee-saved CPU state
 *
 * Each field of this structure holds the value of the register with which it
 * shares a name, such that these values can be restored by a call to
 * longjmp().
 */
typedef struct jmp_buf {
	unsigned long s0, s1, s2, s3, s4, s5, s6, s7;
	unsigned long gp;
	unsigned long sp;
	unsigned long fp;
	unsigned long ra;
} jmp_buf[1];

#endif /* __MIPS_ASM_SETJMP_H__ */
