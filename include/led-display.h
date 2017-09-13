/*
 * (C) Copyright 2005-2010
 * Wolfgang Denk, DENX Software Engineering, wd@denx.de.
 *
 * (C) Copyright 2010
 * Sergei Poselenov, Emcraft Systems, sposelenov@emcraft.com.
 *
 * SPDX-License-Identifier:	GPL-2.0+
 */
#ifndef _led_display_h_
#define _led_display_h_

/* Display Commands */
#define DISPLAY_CLEAR	0x1 /* Clear the display */
#define DISPLAY_HOME	0x2 /* Set cursor at home position */
#define DISPLAY_LINE2	0x4 /* Move to line 2 */

void display_set(int cmd);
int display_putc(char c);

static inline void display_puts(const char *str)
{
	while (str[0])
		display_putc(*str++);
}

static inline void display_sets(const char *str)
{
	display_set(DISPLAY_CLEAR | DISPLAY_HOME);
	display_puts(str);
}

#endif
