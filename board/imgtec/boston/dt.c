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
	u32 upper, build_cfg, ddr_cfg;
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

	/*
	 * Musket adds 0x80000000 to addresses from Xilinx PCIe DMA masters on
	 * some systems (in order to expand the accessible DDR beyond that
	 * available in the 32 bit CPU physical address map, with only a 32 bit
	 * DMA address bus). Indicate that adjustment in dma-ranges.
	 */
	ddr_cfg = readl((u32 *)BOSTON_PLAT_DDRCONF0);
	if (ddr_cfg & (1 << 16)) {
		fdt32_t dma_ranges[6];

		/* DMA address zero */
		dma_ranges[0] = cpu_to_fdt32(0);
		dma_ranges[1] = cpu_to_fdt32(0);

		/* Corresponds to CPU physical address... */
		dma_ranges[2] = cpu_to_fdt32(0);
		dma_ranges[3] = cpu_to_fdt32(0x80000000);

		/* With a size matching that of DDR */
		dma_ranges[4] = cpu_to_fdt32(gd->ram_size >> 32);
		dma_ranges[5] = cpu_to_fdt32(gd->ram_size);

		fdt_setprop(blob, 0, "dma-ranges", &dma_ranges, sizeof(dma_ranges));
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
