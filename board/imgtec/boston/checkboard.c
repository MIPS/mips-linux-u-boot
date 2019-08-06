/*
 * Copyright (C) 2016 Imagination Technologies
 *
 * SPDX-License-Identifier:	GPL-2.0
 */

#include <common.h>

#include <asm/io.h>
#include <asm/mipsregs.h>

#include "boston-lcd.h"
#include "boston-regs.h"

ulong cpu_mips_rtl_changelist(struct udevice *dev)
{
	ulong cl, cl_hex, digit;
	int nibble, i;

	cl_hex = __raw_readl((uint32_t *)BOSTON_PLAT_CORE_CL);

	/*
	 * For some bizarre reason the register contains the changelist when a
	 * hexadecimal display of its value is interpreted as decimal. Convert
	 * that to a standard integer.
	 */
	cl = 0;
	for (nibble = 0; nibble < (sizeof(ulong) * 2); nibble++) {
		digit = (cl_hex >> (nibble * 4)) & 0xf;
		for (i = 0; i < nibble; i++)
			digit *= 10;
		cl += digit;
	}

	return cl;
}

void cpu_mips_rtl_misc(struct udevice *dev, char *buf, int size)
{
	u32 cfg = readl((u32 *)BOSTON_PLAT_BUILDCFG0);
	int idx = 0;

	if (cfg & (BOSTON_PLAT_BUILDCFG0_CFG_NUM | BOSTON_PLAT_BUILDCFG0_CFG_LTR))
		idx += snprintf(&buf[idx], size - idx, " config ");

	if (cfg & BOSTON_PLAT_BUILDCFG0_CFG_NUM)
		idx += snprintf(&buf[idx], size - idx, "%u",
				(cfg & BOSTON_PLAT_BUILDCFG0_CFG_NUM) >> 8);

	if (cfg & BOSTON_PLAT_BUILDCFG0_CFG_LTR)
		idx += snprintf(&buf[idx], size - idx, "%c",
				'a' + ((cfg & BOSTON_PLAT_BUILDCFG0_CFG_LTR) >> 4) - 1);

	if (cfg & BOSTON_PLAT_BUILDCFG0_DP)
		idx += snprintf(&buf[idx], size - idx, ", x%u debug port",
				(cfg & BOSTON_PLAT_BUILDCFG0_DP_MULT) >> 28);
}

int checkboard(void)
{
	lowlevel_display("U-boot  ");

	printf("Board: MIPS Boston\n");

	return 0;
}
