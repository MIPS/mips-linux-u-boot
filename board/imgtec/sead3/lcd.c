/*
 * Copyright (C) 2015 Imagination Technologies
 *
 * SPDX-License-Identifier:	GPL-2.0
 */

#include <common.h>
#include <led-display.h>

#include <asm/io.h>

#include "lcd.h"

enum lcd_write_type {
	LCD_INST,
	LCD_DATA,
};

static struct lcd_regs {
	uint32_t inst;
	uint32_t _pad0;
	uint32_t data;
	uint32_t _pad1;
	uint32_t cpld_status;
	uint32_t _pad2;
	uint32_t cpld_data;
} *regs = (void *)CKSEG1ADDR(0x1f000400);

static void cpld_wait(void)
{
	u8 status;

	do {
		status = __raw_readl(&regs->cpld_status);
	} while (status & 0x1);
}

static void lcd_write(enum lcd_write_type type, u8 val)
{
	u8 status;

	cpld_wait();

	do {
		__raw_readl(&regs->inst);
		cpld_wait();
		status = __raw_readl(&regs->cpld_data);
	} while (status & 0x80);

	switch (type) {
	case LCD_INST:
		__raw_writel(val, &regs->inst);
		break;

	case LCD_DATA:
		__raw_writel(val, &regs->data);
		break;
	}
}

void display_set(int cmd)
{
	unsigned addr = 0;

	if (cmd & DISPLAY_CLEAR)
		lcd_write(LCD_INST, 0x01);

	if (cmd & DISPLAY_HOME)
		lcd_write(LCD_INST, 0x02);

	if (cmd & DISPLAY_LINE2)
		addr += 0x40;

	lcd_write(LCD_INST, 0x80 | addr);
}

int display_putc(char c)
{
	lcd_write(LCD_DATA, c);
	return c;
}
