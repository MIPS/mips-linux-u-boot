/*
 * Copyright (C) 2013 Gabor Juhos <juhosg@openwrt.org>
 * Copyright (C) 2013 Imagination Technologies
 *
 * SPDX-License-Identifier:	GPL-2.0
 */

#include <common.h>
#include <ide.h>
#include <linux/sizes.h>
#include <netdev.h>
#include <pci.h>
#include <pci_gt64120.h>
#include <pci_msc01.h>
#include <rtc.h>
#include <spd.h>

#include <asm/addrspace.h>
#include <asm/io.h>

#include "malta.h"
#include "superio.h"

enum core_card {
	CORE_UNKNOWN,
	CORE_LV,
	CORE_FPGA4,
	CORE_FPGA5,
	CORE_FPGA6,
};

enum sys_con {
	SYSCON_UNKNOWN,
	SYSCON_GT64120,
	SYSCON_MSC01,
};

static enum core_card malta_core_card(void)
{
	u32 corid, rev;
	const void *reg = (const void *)CKSEG1ADDR(MALTA_REVISION);

	rev = __raw_readl(reg);
	corid = (rev & MALTA_REVISION_CORID_MSK) >> MALTA_REVISION_CORID_SHF;

	switch (corid) {
	case MALTA_REVISION_CORID_CORE_LV:
		return CORE_LV;

	case MALTA_REVISION_CORID_CORE_FPGA4:
		return CORE_FPGA4;

	case MALTA_REVISION_CORID_CORE_FPGA5:
		return CORE_FPGA5;

	case MALTA_REVISION_CORID_CORE_FPGA6:
		return CORE_FPGA6;

	default:
		return CORE_UNKNOWN;
	}
}

static enum sys_con malta_sys_con(void)
{
	switch (malta_core_card()) {
	case CORE_LV:
		return SYSCON_GT64120;

	case CORE_FPGA4 ... CORE_FPGA6:
		return SYSCON_MSC01;

	default:
		return SYSCON_UNKNOWN;
	}
}

phys_size_t initdram(int board_type)
{
	extern unsigned char malta_spd_read(unsigned int offset);
	unsigned int mem_type, nrow_addr, ncol_addr, nrows, nbanks;
	phys_size_t sz;

	/* read values from SPD */
	mem_type = malta_spd_read(SPD_MEM_TYPE);
	nrow_addr = malta_spd_read(SPD_NROW_ADDR) & 0xf;
	ncol_addr = malta_spd_read(SPD_NCOL_ADDR) & 0xf;
	nrows = malta_spd_read(SPD_NROWS);
	nbanks = malta_spd_read(SPD_NBANKS);

	/* handle DDR2 row encoding */
	if (mem_type >= SPD_MEMTYPE_DDR2)
		nrows = (nrows & 0x7) + 1;

	/* calculate RAM size */
	sz = (phys_size_t)nrows << (nrow_addr + ncol_addr);
	sz *= nbanks;
	sz *= 8;

	return sz;
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

int checkboard(void)
{
	enum core_card core;

	malta_lcd_puts("U-Boot");
	puts("Board: MIPS Malta");

	core = malta_core_card();
	switch (core) {
	case CORE_LV:
		puts(" CoreLV");
		break;

	case CORE_FPGA4 ... CORE_FPGA6:
		printf(" CoreFPGA%d", 4 + (core - CORE_FPGA4));
		break;

	default:
		puts(" CoreUnknown");
	}

	putc('\n');
	return 0;
}

int board_eth_init(bd_t *bis)
{
	return pci_eth_init(bis);
}

void _machine_restart(void)
{
	void __iomem *reset_base;

	reset_base = (void __iomem *)CKSEG1ADDR(MALTA_RESET_BASE);
	__raw_writel(GORESET, reset_base);
	mdelay(1000);
}

int board_early_init_f(void)
{
	ulong io_base;

	/* choose correct PCI I/O base */
	switch (malta_sys_con()) {
	case SYSCON_GT64120:
		io_base = CKSEG1ADDR(MALTA_GT_PCIIO_BASE);
		break;

	case SYSCON_MSC01:
		io_base = CKSEG1ADDR(MALTA_MSC01_PCIIO_BASE);
		break;

	default:
		return -1;
	}

	set_io_port_base(io_base);

	/* setup FDC37M817 super I/O controller */
	malta_superio_init();

	return 0;
}

static int read_eeprom(void)
{
	uchar buf[256], ver, count, id, size;
	unsigned i, sn_pos, pos = 0x200;
	char sn[16];
	int err;

	err = eeprom_read(0x54, pos++, &ver, 1);
	if (err)
		return err;

	if (ver != 1) {
		printf("WARNING: EEPROM data version %u unsupported\n", ver);
		return -1;
	}

	err = eeprom_read(0x54, pos++, &count, 1);
	if (err)
		return err;

	while (count--) {
		err = eeprom_read(0x54, pos++, &id, 1);
		if (err)
			return err;

		err = eeprom_read(0x54, pos++, &size, 1);
		if (err)
			return err;

		err = eeprom_read(0x54, pos, buf, size);
		if (err)
			return err;
		pos += size;

		switch (id) {
		case 1:
			/* MAC */
			eth_setenv_enetaddr("ethaddr", buf);
			break;

		case 2:
			/* Serial */
			i = 0;
			while (!buf[i] && (i < size))
				i++;

			snprintf(sn, sizeof(sn), "ERROR");
			sn_pos = 0;
			for (; i < size; i++)
				sn_pos += snprintf(&sn[sn_pos],
						   sizeof(sn) - sn_pos,
						   "%x", buf[i]);

			setenv("serial#", sn);
			break;

		default:
			printf("WARNING: unrecognised EEPROM data ID %u\n", id);
		}
	}

	return 0;
}

int misc_init_r(void)
{
	rtc_reset();

	/* Read the MAC & serial from EEPROM */
	if (!getenv("ethaddr") || !getenv("serial#"))
		read_eeprom();

	return 0;
}

void pci_init_board(void)
{
	pci_dev_t bdf;
	u32 val32;
	u8 val8;

	switch (malta_sys_con()) {
	case SYSCON_GT64120:
		gt64120_pci_init((void *)CKSEG1ADDR(MALTA_GT_BASE),
				 0x00000000, 0x00000000, CONFIG_SYS_MEM_SIZE,
				 0x10000000, 0x10000000, 128 * 1024 * 1024,
				 0x00000000, 0x00000000, 0x20000);
		break;

	default:
	case SYSCON_MSC01:
		msc01_pci_init((void *)CKSEG1ADDR(MALTA_MSC01_PCI_BASE),
			       0x00000000, 0x00000000, CONFIG_SYS_MEM_SIZE,
			       MALTA_MSC01_PCIMEM_MAP,
			       CKSEG1ADDR(MALTA_MSC01_PCIMEM_BASE),
			       MALTA_MSC01_PCIMEM_SIZE, MALTA_MSC01_PCIIO_MAP,
			       0x00000000, MALTA_MSC01_PCIIO_SIZE);
		break;
	}

	bdf = pci_find_device(PCI_VENDOR_ID_INTEL,
			      PCI_DEVICE_ID_INTEL_82371AB_0, 0);
	if (bdf == -1)
		panic("Failed to find PIIX4 PCI bridge\n");

	/* setup PCI interrupt routing */
	pci_write_config_byte(bdf, PCI_CFG_PIIX4_PIRQRCA, 10);
	pci_write_config_byte(bdf, PCI_CFG_PIIX4_PIRQRCB, 10);
	pci_write_config_byte(bdf, PCI_CFG_PIIX4_PIRQRCC, 11);
	pci_write_config_byte(bdf, PCI_CFG_PIIX4_PIRQRCD, 11);

	/* mux SERIRQ onto SERIRQ pin */
	pci_read_config_dword(bdf, PCI_CFG_PIIX4_GENCFG, &val32);
	val32 |= PCI_CFG_PIIX4_GENCFG_SERIRQ;
	pci_write_config_dword(bdf, PCI_CFG_PIIX4_GENCFG, val32);

	/* enable SERIRQ - Linux currently depends upon this */
	pci_read_config_byte(bdf, PCI_CFG_PIIX4_SERIRQC, &val8);
	val8 |= PCI_CFG_PIIX4_SERIRQC_EN | PCI_CFG_PIIX4_SERIRQC_CONT;
	pci_write_config_byte(bdf, PCI_CFG_PIIX4_SERIRQC, val8);

	bdf = pci_find_device(PCI_VENDOR_ID_INTEL,
			      PCI_DEVICE_ID_INTEL_82371AB, 0);
	if (bdf == -1)
		panic("Failed to find PIIX4 IDE controller\n");

	/* enable bus master & IO access */
	val32 |= PCI_COMMAND_MASTER | PCI_COMMAND_IO;
	pci_write_config_dword(bdf, PCI_COMMAND, val32);

	/* set latency */
	pci_write_config_byte(bdf, PCI_LATENCY_TIMER, 0x40);

	/* enable IDE/ATA */
	pci_write_config_dword(bdf, PCI_CFG_PIIX4_IDETIM_PRI,
			       PCI_CFG_PIIX4_IDETIM_IDE);
	pci_write_config_dword(bdf, PCI_CFG_PIIX4_IDETIM_SEC,
			       PCI_CFG_PIIX4_IDETIM_IDE);
}
