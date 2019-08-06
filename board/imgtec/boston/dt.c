/*
 * Copyright (C) 2016 Imagination Technologies
 *
 * SPDX-License-Identifier:	GPL-2.0
 */

#include <common.h>
#include <fdt_support.h>
#include <asm/io.h>
#include "boston-regs.h"

int ft_board_setup(void *blob, bd_t *bd)
{
	DECLARE_GLOBAL_DATA_PTR;
	u64 mem_start[2], mem_size[2];
	int mem_regions;
	fdt32_t prop_val;

	if (gd->flags & GD_FLG_COHERENT_DMA) {
		prop_val = cpu_to_fdt32(1);
		fdt_setprop(blob, 0, "dma-coherent", &prop_val, sizeof(prop_val));
	} else {
		fdt_delprop(blob, 0, "dma-coherent");
	}

	mem_start[0] = 0;
	mem_size[0] = min_t(u64, 256llu << 20, gd->ram_size);
	mem_regions = 1;

	if (gd->ram_size > mem_size[0]) {
		mem_start[1] = 0x80000000 + mem_size[0];
		mem_size[1] = gd->ram_size - mem_size[0];
		mem_regions++;
	}

	return fdt_fixup_memory_banks(blob, mem_start, mem_size, mem_regions);
}
