/*
 * SEAD-3 Board DDR Initialisation
 *
 * Copyright (c) 2017 Imagination Technologies Ltd.
 *
 * SPDX-License-Identifier:	GPL-2.0+
 *
 */

#include <common.h>

int dram_init(void)
{
	DECLARE_GLOBAL_DATA_PTR;

	gd->ram_size = 128 << 20;

	return 0;
}
