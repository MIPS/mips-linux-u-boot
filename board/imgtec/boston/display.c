/*
 * Copyright (C) 2017 Imagination Technologies
 *
 * SPDX-License-Identifier:	GPL-2.0
 */

#include <common.h>
#include <led-display.h>

#include <asm/io.h>

#include "boston-lcd.h"

static char buf[8];
static int pos;

void display_set(int cmd)
{
	if (cmd & DISPLAY_CLEAR)
		memset(buf, ' ', sizeof(buf));

	if (cmd & DISPLAY_HOME)
		pos = 0;

	lowlevel_display(buf);
}

int display_putc(char c)
{
	if (pos >= 8)
		return -1;

	buf[pos++] = c;
	lowlevel_display(buf);
	return c;
}
