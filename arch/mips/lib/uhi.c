/*
 * MIPS Unified Hosting Interface (UHI) Support
 *
 * Copyright (c) 2016 Imagination Technologies Ltd.
 *
 * SPDX-License-Identifier:	GPL-2.0+
 */

#include <common.h>
#include <asm/compiler.h>
#include <asm/mipsregs.h>
#include <asm/uhi.h>
#include <cli.h>

enum uhi_op {
	MIPS_UHI_EXIT = 1,
	MIPS_UHI_CLOSE = 3,
	MIPS_UHI_READ,
	MIPS_UHI_WRITE,
	MIPS_UHI_FSTAT = 8,
};

int mips_uhi_handle(struct pt_regs *regs)
{
	unsigned long file, count, sz;
	unsigned short insn;
	char *ch;

	switch (regs->regs[25]) {
	case MIPS_UHI_EXIT:
		set_return_code(regs->regs[4]);
		cli_longjmp();
		break;

	case MIPS_UHI_READ:
		file = regs->regs[4];
		ch = (char *)regs->regs[5];
		sz = regs->regs[6];

		if (file != stdin)
			goto unhandled;

		count = 0;
		while (sz--) {
			if (!ftstc(file))
				break;

			*ch++ = fgetc(file);
			count++;
		}

		regs->regs[2] = count;
		break;

	case MIPS_UHI_WRITE:
		file = regs->regs[4];
		ch = (char *)regs->regs[5];
		sz = regs->regs[6];

		if (file != stdout && file != stderr)
			goto unhandled;

		while (sz--)
			fputc(file, *ch++);

		regs->regs[2] = sz;
		break;

	case MIPS_UHI_FSTAT:
	case MIPS_UHI_CLOSE:
		file = regs->regs[4];

		if (file > stderr)
			goto unhandled;

		regs->regs[2] = -1;
		regs->regs[3] = 0;
		break;

	default:
unhandled:
		asm volatile(
			"li	$" MIPS_R_PFX "2, %7\n"
		"	move	$" MIPS_R_PFX "4, %2\n"
		"	move	$" MIPS_R_PFX "5, %3\n"
		"	move	$" MIPS_R_PFX "6, %4\n"
		"	move	$" MIPS_R_PFX "7, %5\n"
		"	move	$" MIPS_R_PFX "25, %6\n"
		"	sdbbp	1\n"
		"	move	%0, $" MIPS_R_PFX "2\n"
		"	move	%1, $" MIPS_R_PFX "3"
		: "=r"(regs->regs[2]), "=r"(regs->regs[3])
		: "r"(regs->regs[4]), "r"(regs->regs[5]),
		  "r"(regs->regs[6]), "r"(regs->regs[7]),
		  "r"(regs->regs[25]), "i"(MIPS_UHI_SYSCALL)
		: "memory");
	}

	/* advance past the syscall instruction */
	if (IS_ENABLED(__nanomips__)) {
		insn = *(unsigned short *)regs->cp0_epc;

		if (insn & BIT(12))
			regs->cp0_epc += 2;
		else
			regs->cp0_epc += 4;
	} else {
		regs->cp0_epc += 4;
	}

	/* return indicating success */
	return 0;
}
