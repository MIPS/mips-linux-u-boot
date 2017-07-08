/*
 * Simple LCD Displays
 *
 * Copyright (c) 2017 Imagination Technologies Ltd.
 *
 * SPDX-License-Identifier:	GPL-2.0+
 */

#include <common.h>
#include <dm.h>
#include <dm/device-internal.h>
#include <led-display.h>
#include <version.h>

struct lcd_display_priv {
	unsigned int pos_x;
	unsigned int pos_y;
};

static void _display_set(struct udevice *dev, int cmd)
{
	const struct lcd_display_ops *ops;
	struct lcd_display_priv *priv;

	priv = dev_get_uclass_priv(dev);

	if (cmd & DISPLAY_HOME) {
		priv->pos_x = 0;
		priv->pos_y = 0;
	}

	if (cmd & DISPLAY_NEWLINE) {
		priv->pos_x = 0;
		priv->pos_y++;
	}

	ops = dev_get_driver_ops(dev);
	if (ops->set)
		ops->set(dev, cmd);
}

void display_set(int cmd)
{
	struct udevice *dev;

	for (uclass_first_device(UCLASS_LCD, &dev);
	     dev; uclass_next_device (&dev))
		_display_set(dev, cmd);
}

static void _display_putc(struct udevice *dev, char c)
{
	struct lcd_display_uc_platdata *pdata;
	const struct lcd_display_ops *ops;
	struct lcd_display_priv *priv;

	priv = dev_get_uclass_priv(dev);
	pdata = dev_get_uclass_platdata(dev);

	if (priv->pos_y >= pdata->rows)
		return;

	if (c == '\n') {
		_display_set(dev, DISPLAY_NEWLINE);
		return;
	}

	if (priv->pos_x >= pdata->columns)
		return;

	ops = dev_get_driver_ops(dev);
	if (ops->putc)
		ops->putc(dev, c);

	priv->pos_x++;
}

int display_putc(char c)
{
	struct udevice *dev;

	for (uclass_first_device(UCLASS_LCD, &dev);
	     dev; uclass_next_device (&dev))
		_display_putc(dev, c);

	return 0;
}

#if 1

static void _display_puts(struct udevice *dev, const char *str)
{
	while (*str)
		_display_putc(dev, *str++);
}

static int lcd_post_bind(struct udevice *dev)
{
	struct lcd_display_uc_platdata *pdata;
	const char *ver = "v2017.07-rc2";
	int err;

	err = device_probe(dev);
	if (err)
		return err;

	_display_set(dev, DISPLAY_CLEAR | DISPLAY_HOME);
	_display_puts(dev, "U-Boot");

	pdata = dev_get_uclass_platdata(dev);
	if (pdata->columns < (6 + 1 + strlen(ver))) {
		if (pdata->rows < 2)
			return 0;

		_display_set(dev, DISPLAY_NEWLINE);
	} else {
		_display_putc(dev, ' ');
	}

	_display_puts(dev, ver);

	return 0;
}

#endif

UCLASS_DRIVER(lcd) = {
	.id		= UCLASS_LCD,
	.name		= "lcd",
	.per_device_auto_alloc_size = sizeof(struct lcd_display_priv),
	.per_device_platdata_auto_alloc_size = sizeof(struct lcd_display_uc_platdata),
#if 1
	.post_bind	= lcd_post_bind,
#endif
};
