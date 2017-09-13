/*
 * Copyright (C) 2015 Imagination Technologies
 * Author: Paul Burton <paul.burton@imgtec.com>
 *
 * SPDX-License-Identifier:	GPL-2.0+
 */

#ifndef __ARCH_MIPS_ASM_MAAR_H__
#define __ARCH_MIPS_ASM_MAAR_H__

#include <asm/mipsregs.h>
#include <asm/types.h>

struct mips_maar_cfg {
	phys_addr_t lower;
	phys_addr_t upper;
	unsigned attrs;
};

extern void mips_maar_init(struct mips_maar_cfg *cfg);

#endif /* __ARCH_MIPS_ASM_MAAR_H__ */
