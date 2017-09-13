/*
 * Copyright (C) 2015 Imagination Technologies
 *
 * SPDX-License-Identifier:	GPL-2.0
 */

#include <common.h>
#include <bootstage.h>
#include <led-display.h>
#include <cpu.h>
#include <netdev.h>
#include <dm.h>

#include <asm/io.h>
#include <asm/processor.h>

#include "sram.h"

int checkboard(void)
{
	struct udevice *cpu_dev;
	char cpu_name[11];
	u32 err;

	display_sets("U-Boot SEAD3");
	display_set(DISPLAY_LINE2);
	display_puts("CPU: ");

	err = uclass_first_device_err(UCLASS_CPU, &cpu_dev);
	if (err)
		strcpy(cpu_name,"Unknown");

	err = cpu_get_desc(cpu_dev, cpu_name, sizeof(cpu_name));
	if (err)
		strcpy(cpu_name,"Unknown");

	display_puts(cpu_name);

	display_sram(0);
	return 0;
}

void show_boot_progress(int status)
{
	if (status == BOOTSTAGE_ID_RUN_OS)
		display_sets("Booting OS");
}

int board_eth_init(bd_t *bis)
{
	return smc911x_initialize(0, CONFIG_SMC911X_BASE);
}

void _machine_restart(void)
{
	void __iomem *reset_base = (void __iomem *)CKSEG1ADDR(0x1f000050);

	__raw_writel(0x4d, reset_base);
	mdelay(1000);
}
