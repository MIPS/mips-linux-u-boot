/*
 * MIPS Unified Hosting Interface (UHI) Support
 *
 * Copyright (c) 2016 Imagination Technologies Ltd.
 *
 * SPDX-License-Identifier:	GPL-2.0+
 */

#ifndef __MIPS_UHI_H__
#define __MIPS_UHI_H__

#include <asm/ptrace.h>
#include <errno.h>

#define MIPS_UHI_SYSCALL 1

#ifdef CONFIG_MIPS_UHI
extern int mips_uhi_handle(struct pt_regs *regs);
#else
static inline int mips_uhi_handle(struct pt_regs *regs)
{
	return -ENOSYS;
}
#endif

#endif /* __MIPS_UHI_H__ */
