/*
 * Copyright (c) 2016 Imagination Technologies
 *
 * SPDX-License-Identifier:	GPL-2.0+
 */
#include <config.h>
#include <asm/asm.h>
#include <asm/regdef.h>
#include <generated/asm-offsets.h>

LEAF(setjmp)
	PTR_S	s0, JMPBUF_S0(a0)
	PTR_S	s1, JMPBUF_S1(a0)
	PTR_S	s2, JMPBUF_S2(a0)
	PTR_S	s3, JMPBUF_S3(a0)
	PTR_S	s4, JMPBUF_S4(a0)
	PTR_S	s5, JMPBUF_S5(a0)
	PTR_S	s6, JMPBUF_S6(a0)
	PTR_S	s7, JMPBUF_S7(a0)
	PTR_S	gp, JMPBUF_GP(a0)
	PTR_S	sp, JMPBUF_SP(a0)
	PTR_S	fp, JMPBUF_FP(a0)
	PTR_S	ra, JMPBUF_RA(a0)
	move	va0, zero
	jr	ra
	END(setjmp)

LEAF(longjmp)
	PTR_L	s0, JMPBUF_S0(a0)
	PTR_L	s1, JMPBUF_S1(a0)
	PTR_L	s2, JMPBUF_S2(a0)
	PTR_L	s3, JMPBUF_S3(a0)
	PTR_L	s4, JMPBUF_S4(a0)
	PTR_L	s5, JMPBUF_S5(a0)
	PTR_L	s6, JMPBUF_S6(a0)
	PTR_L	s7, JMPBUF_S7(a0)
	PTR_L	gp, JMPBUF_GP(a0)
	PTR_L	sp, JMPBUF_SP(a0)
	PTR_L	fp, JMPBUF_FP(a0)
	PTR_L	ra, JMPBUF_RA(a0)
	move	va0, a1
	jr	ra
	END(longjmp)
