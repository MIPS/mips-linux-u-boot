/*
 * MIPS Relocations
 *
 * Copyright (c) 2017 Imagination Technologies Ltd.
 *
 * SPDX-License-Identifier:	GPL-2.0+
 */

#ifndef __ASM_MIPS_RELOCS_H__
#define __ASM_MIPS_RELOCS_H__

#define R_MIPS_NONE		0
#define R_MIPS_32		2
#define R_MIPS_26		4
#define R_MIPS_HI16		5
#define R_MIPS_LO16		6
#define R_MIPS_PC16		10
#define R_MIPS_64		18
#define R_MIPS_HIGHER		28
#define R_MIPS_HIGHEST		29
#define R_MIPS_PC21_S2		60
#define R_MIPS_PC26_S2		61

#define R_NANOMIPS_32		1
#define R_NANOMIPS_64		2
#define R_NANOMIPS_PC25_S1	13
#define R_NANOMIPS_PC21_S1	14
#define R_NANOMIPS_PC14_S1	15
#define R_NANOMIPS_PC11_S1	16
#define R_NANOMIPS_PC10_S1	17
#define R_NANOMIPS_PC7_S1	18
#define R_NANOMIPS_PC_HI20	27
#define R_NANOMIPS_HI20		28
#define R_NANOMIPS_LO12		29
#define R_NANOMIPS_ALIGN	64

#endif /* __ASM_MIPS_RELOCS_H__ */
