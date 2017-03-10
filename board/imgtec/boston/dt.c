/*
 * Copyright (C) 2016 Imagination Technologies
 *
 * SPDX-License-Identifier:	GPL-2.0
 */

#include <common.h>
#include <fdt_support.h>
#include <asm/io.h>
#include "boston-regs.h"

static const char sleipnir_compat[] = "img,boston-sleipnir\0img,boston";

const void *get_best_fit_compat(const void *fdt, int *lenp)
{
	if (detect_sleipnir()) {
		if (lenp)
			*lenp = sizeof(sleipnir_compat);
		return sleipnir_compat;
	}

	return fdt_getprop(fdt, 0, "compatible", lenp);
}

int ft_board_setup(void *blob, bd_t *bd)
{
	DECLARE_GLOBAL_DATA_PTR;
	u64 mem_start[2], mem_size[2];
	int mem_regions, off;
	bool coherent, enabled;
	ulong phys;
	u32 upper, build_cfg;
	const fdt32_t *reg;
	fdt32_t prop_val;

	build_cfg = readl((u32 *)BOSTON_PLAT_BUILDCFG0);

	off = fdt_node_offset_by_compatible(blob, -1, "xlnx,axi-pcie-host-1.00.a");
	while (off != -FDT_ERR_NOTFOUND) {
		coherent = false;
		enabled = false;

		reg = fdt_getprop(blob, off, "reg", NULL);
		if (reg) {
			phys = fdt_translate_address(blob, off, reg);
			switch (phys) {
			case 0x10000000: /* PCIe0 */
				upper = readl((u32 *)BOSTON_PLAT_NOCPCIE0ADDR);

				if (detect_sleipnir())
					enabled = false;
				else
					enabled = build_cfg & BOSTON_PLAT_BUILDCFG0_PCIE0;
				break;

			case 0x12000000: /* PCIe1 */
				upper = readl((u32 *)BOSTON_PLAT_NOCPCIE1ADDR);
				enabled = build_cfg & BOSTON_PLAT_BUILDCFG0_PCIE1;
				break;

			case 0x14000000: /* PCIe2 */
				upper = readl((u32 *)BOSTON_PLAT_NOCPCIE2ADDR);
				enabled = build_cfg & BOSTON_PLAT_BUILDCFG0_PCIE2;
				break;

			default:
				upper = 0;
			}

			coherent = upper == 0x10;
		}

		if (coherent) {
			prop_val = cpu_to_fdt32(1);
			fdt_setprop(blob, off, "dma-coherent", &prop_val, sizeof(prop_val));
		} else {
			fdt_delprop(blob, off, "dma-coherent");
		}

		fdt_setprop_string(blob, off, "status",
				   enabled ? "okay" : "disabled");

		off = fdt_node_offset_by_compatible(blob, off, "xlnx,axi-pcie-host-1.00.a");
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
