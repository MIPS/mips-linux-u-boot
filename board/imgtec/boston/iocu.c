/*
 * Copyright (C) 2016 Imagination Technologies
 *
 * SPDX-License-Identifier:	GPL-2.0
 */

#include <common.h>
#include <dm.h>
#include <env_callback.h>
#include <asm/cm.h>
#include "boston-regs.h"

static const struct mmio_region mmio_regions[] = {
	{ 0x10000000, 0x160f0000, .enable = 1 },
	{ 0x17ff0000, 0x17ff0000, .enable = 1 },
	{ 0 },
};

const struct mmio_region *get_mmio_regions(void)
{
	return mmio_regions;
}

bool plat_iocu_usable(unsigned int cluster, unsigned int iocu)
{
	u32 buildcfg = readl((u32 *)BOSTON_PLAT_BUILDCFG0);

	return !!(buildcfg & BOSTON_PLAT_BUILDCFG0_IOCU);
}

static int set_io_coherent(bool coherent)
{
	DECLARE_GLOBAL_DATA_PTR;

	if (!coherent) {
		printf("I/O:   Non-Coherent (Forced by environment)\n");
		goto noncoherent;
	}

	if (gd->arch.num_iocus_usable < 0) {
		printf("I/O:   Non-Coherent (IOCU init error %d)\n",
		       gd->arch.num_iocus_usable);
		goto noncoherent;
	}

	if (gd->arch.num_iocus == 0) {
		printf("I/O:   Non-Coherent (No IOCU)\n");
		goto noncoherent;
	}

	if (gd->arch.num_iocus_usable == 0) {
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
