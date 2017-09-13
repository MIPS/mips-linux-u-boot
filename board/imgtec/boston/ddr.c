/*
 * Copyright (C) 2016 Imagination Technologies
 *
 * SPDX-License-Identifier:	GPL-2.0
 */

#include <common.h>
#include <linux/sizes.h>

#include <asm/io.h>
#include <asm/maar.h>

#include "boston-regs.h"

phys_size_t initdram(int board_type)
{
	u32 ddrconf0 = readl((uint32_t *)BOSTON_PLAT_DDRCONF0);
	struct mips_maar_cfg maar_cfg[] = {
		{ 0x00000000ull, 0x0fffffffull, MIPS_MAAR_S | MIPS_MAAR_V },
		{ 0x80000000ull, 0xffffffffull, MIPS_MAAR_S | MIPS_MAAR_V },
		{ },
	};
	phys_size_t ddr_size;

	ddr_size = (phys_size_t)(ddrconf0 & BOSTON_PLAT_DDRCONF0_SIZE) << 30;

	maar_cfg[1].upper = maar_cfg[1].lower + ddr_size - 1;
	mips_maar_init(maar_cfg);

	return ddr_size;
}

ulong board_get_usable_ram_top(ulong total_size)
{
	DECLARE_GLOBAL_DATA_PTR;

	if (gd->ram_top < (ulong)phys_to_virt(CONFIG_SYS_SDRAM_BASE)) {
		/* 2GB wrapped around to 0 */
		return (ulong)phys_to_virt(SZ_256M);
	}

	return min_t(ulong, gd->ram_top, (ulong)phys_to_virt(SZ_256M));
}
