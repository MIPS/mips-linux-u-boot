/*
 * Copyright (C) 2015 Imagination Technologies
 *
 * SPDX-License-Identifier:	GPL-2.0
 */

#ifndef _SEAD3_SRAM_H
#define _SEAD3_SRAM_H

enum sram_type {
	ISRAM,
	DSRAM,
};

#define SRAM_CFG_PHYS		(0x3fffff << 10)
#define SRAM_CFG_R		(1 << 1)
#define SRAM_CFG_E		(1 << 0)

extern void display_sram(int flags);
#define DISP_ABSENT		(1 << 0)

extern int move_sram(enum sram_type type, phys_addr_t base);

extern int write_sram(void *dst, void *src, size_t sz);

#endif /* _SEAD3_SRAM_H */
