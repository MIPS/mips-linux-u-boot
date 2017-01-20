/*
 * Copyright (C) 2016 Imagination Technologies
 *
 * SPDX-License-Identifier:	GPL-2.0
 */

#include <common.h>
#include <dm.h>
#include <env_callback.h>
#include <asm/cm.h>

#include "malta.h"

static int set_io_coherent(bool coherent)
{
	DECLARE_GLOBAL_DATA_PTR;
	u32 cfg0;

	if (!coherent) {
		printf("I/O:   Non-Coherent (Forced by environment)\n");
		goto noncoherent;
	}

	if (gd->arch.num_iocus < 0) {
		printf("I/O:   Non-Coherent (IOCU init error %d)\n",
		       gd->arch.num_iocus);
		goto noncoherent;
	}

	if (gd->arch.num_iocus == 0) {
		printf("I/O:   Non-Coherent (No IOCU)\n");
		goto noncoherent;
	}

	cfg0 = __raw_readl((u32 *)CKSEG1ADDR(MALTA_DDR_CTRL));
	if (!(cfg0 & MALTA_ROCIT_CFG0_PCI_IOCU)) {
		printf("I/O:   Non-Coherent (Set by switch)\n");
		goto noncoherent;
	}

	/* Record that I/O is coherent */
	gd->flags |= GD_FLG_COHERENT_DMA;

	printf("I/O:   Coherent\n");
	return 0;

noncoherent:
	cfg0 = __raw_readl((u32 *)CKSEG1ADDR(MALTA_DDR_CTRL));
	cfg0 &= ~MALTA_ROCIT_CFG0_PCI_IOCU;
	__raw_writel(cfg0, (u32 *)CKSEG1ADDR(MALTA_DDR_CTRL));

	/* Record that I/O is not coherent */
	gd->flags &= ~GD_FLG_COHERENT_DMA;

	return 0;
}

static int on_io_coherent(const char *name, const char *value,
			  enum env_op op, int flags)
{
	switch (op) {
	case env_op_create:
	case env_op_overwrite:
		if (!strcmp(value, "0")) {
			set_io_coherent(false);
		} else if (!strcmp(value, "1")) {
			set_io_coherent(true);
		} else {
			printf("## io.coherent must equal 0 or 1\n");
			return -EINVAL;
		}
		return 0;

	case env_op_delete:
		set_io_coherent(true);
		return 0;

	default:
		return 0;
	}
}
U_BOOT_ENV_CALLBACK(io_coherent, on_io_coherent);

int misc_init_f(void)
{
	return set_io_coherent(getenv_yesno("io.coherent") != 0);
}
