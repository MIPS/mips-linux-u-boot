/*
 * Copyright (C) 2016 Imagination Technologies
 *
 * SPDX-License-Identifier:	GPL-2.0
 */

#include <common.h>
#include <asm/addrspace.h>

void board_detail(void)
{
	const unsigned char *bitfile_cfg = (void *)CKSEG0ADDR(0x1dfc0000);
	unsigned char ch;

	printf("\nBitfile information from flash:\n  ");

	while (true) {
		ch = *bitfile_cfg++;
		if (ch == 0 || ch == 0xff)
			break;
		putc(ch);
		if (ch == '\n')
			puts("  ");
	}

	putc('\n');
}
