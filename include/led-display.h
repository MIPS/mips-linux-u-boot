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

struct lcd_display_ops {
	void (*set)(struct udevice *dev, int cmd);
	void (*putc)(struct udevice *dev, char c);
};

struct lcd_display_uc_platdata {
	unsigned int rows;
	unsigned int columns;
};

/* Display Commands */
#define DISPLAY_CLEAR	0x1 /* Clear the display */
#define DISPLAY_HOME	0x2 /* Set cursor at home position */
#define DISPLAY_NEWLINE	0x4 /* Move cursor to a new line */

void display_set(int cmd);
int display_putc(char c);

#endif
