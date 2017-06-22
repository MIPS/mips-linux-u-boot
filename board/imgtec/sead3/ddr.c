/*
 * Copyright (C) 2015 Imagination Technologies
 *
 * SPDX-License-Identifier:	GPL-2.0
 */

#include <common.h>
#include <ddr_spd.h>
#include <led-display.h>
#include <asm/io.h>

#include <linux/sizes.h>

u32 __iomem *spd_config = (u32 __iomem *)CKSEG1ADDR(0x1b000040);
u32 __iomem *spd_addr = (u32 __iomem *)CKSEG1ADDR(0x1b000048);
u32 __iomem *spd_data = (u32 __iomem *)CKSEG1ADDR(0x1b000050);

#define SPD_DATA_ERROR	(0x1 << 9)
#define SPD_DATA_BUSY	(0x1 << 8)
#define SPD_DATA_DATA	(0xff << 0)

static u8 spd_read(unsigned off)
{
	u32 data;

	__raw_writel(off, spd_addr);

	do {
		data = __raw_readl(spd_data);
	} while (data & SPD_DATA_BUSY);

	if (data & SPD_DATA_ERROR) {
		display_sets("SPD read error");
		hang();
	}

	return data & SPD_DATA_DATA;
}

phys_size_t initdram(int board_type)
{
	struct ddr2_spd_eeprom_s spd;
	unsigned rows_a, cols_a;

	__raw_writel(1000, spd_config);

#define read_field(name)	\
	spd.name = spd_read(offsetof(struct ddr2_spd_eeprom_s, name))

	read_field(mem_type);
	read_field(nrow_addr);
	read_field(ncol_addr);
	read_field(dataw);
	read_field(nbanks);

#undef read_field

	if (spd.mem_type != SPD_MEMTYPE_DDR2) {
		display_sets("DIMM not DDR2");
		hang();
	}

	if ((spd.dataw | 0x8) != 0x48) {
		display_sets("DIMM bad width");
		hang();
	}

	rows_a = 1 << (spd.nrow_addr & 0xf);
	cols_a = 1 << (spd.ncol_addr & 0xf);

	return 8 * spd.nbanks * rows_a * cols_a;
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
