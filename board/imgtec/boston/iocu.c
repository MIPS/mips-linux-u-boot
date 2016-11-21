/*
 * Copyright (C) 2017 MIPS Technologies
 *
 * SPDX-License-Identifier:	GPL-2.0
 */

#include <common.h>
#include <dm.h>
#include <env_callback.h>
#include <asm/cm.h>
#include "boston-regs.h"

static int set_io_coherent(bool coherent)
{
	DECLARE_GLOBAL_DATA_PTR;
	u32 build_cfg, rev;

	if (!coherent) {
		printf("I/O:   Non-Coherent (Forced by environment)\n");
		goto noncoherent;
	}

	if (mips_cm_num_iocus() == 0) {
		printf("I/O:   Non-Coherent (No IOCU)\n");
		goto noncoherent;
	}

	build_cfg = readl((u32 *)BOSTON_PLAT_BUILDCFG0);
	if (!(build_cfg & BOSTON_PLAT_BUILDCFG0_IOCU)) {
		printf("I/O:   Non-Coherent (IOCU not connected)\n");
		goto noncoherent;
	}

	/*
	 * We have some number of connected IOCUs. Map all PCIe DMA access to
	 * hit the IOCU by offsetting the addresses as they pass from the PCIe
	 * controller to the NoC.
	 */
	writel(0x10, (u32 *)BOSTON_PLAT_NOCPCIE0ADDR);
	writel(0x10, (u32 *)BOSTON_PLAT_NOCPCIE1ADDR);
	writel(0x10, (u32 *)BOSTON_PLAT_NOCPCIE2ADDR);

	/* Configure MMIO writes */
	rev = __raw_readl(mips_cm_base() + GCR_REV);
	if (rev < GCR_REV_CM3) {
		__raw_writel(0x10000000, mips_cm_base() + GCR_REG0_BASE);
		__raw_writel(0xf8000002, mips_cm_base() + GCR_REG0_MASK);
	} else {
		/* TODO */
	}

	/* Configure IOCU bus AxCACHE values */
	clrsetbits_le32((u32 *)BOSTON_PLAT_SPARE0,
			BOSTON_PLAT_SPARE0_IC_ARCACHE |
			BOSTON_PLAT_SPARE0_IC_AWCACHE,
			BOSTON_PLAT_SPARE0_IC_ARCACHE_COH_NOALLOC |
			BOSTON_PLAT_SPARE0_IC_AWCACHE_COH_ALLOC);

	/* Record that I/O is coherent */
	gd->flags |= GD_FLG_COHERENT_DMA;

	printf("I/O:   Coherent\n");
	return 0;

noncoherent:
	/* Map all PCIe DMA access to its default, non-IOCU, target */
	writel(0x00, (u32 *)BOSTON_PLAT_NOCPCIE0ADDR);
	writel(0x00, (u32 *)BOSTON_PLAT_NOCPCIE1ADDR);
	writel(0x00, (u32 *)BOSTON_PLAT_NOCPCIE2ADDR);

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
	return set_io_coherent(env_get_yesno("io.coherent") != 0);
}
