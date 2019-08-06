/*
 * SEAD-3 LCD Display Driver
 *
 * Copyright (c) 2017 Imagination Technologies Ltd.
 *
 * SPDX-License-Identifier:	GPL-2.0+
 */

#include <common.h>
#include <dm.h>
#include <led-display.h>
#include <regmap.h>
#include <syscon.h>

struct sead3_lcd_priv {
	struct regmap *regmap;
	unsigned int offset;
};

enum sead3_lcd_reg {
	REG_INSN		= 0x00,
	REG_DATA		= 0x08,
	REG_CPLD_STAT		= 0x10,
#define REG_CPLD_STAT_BUSY	BIT(0)
	REG_CPLD_DATA		= 0x18,
#define REG_CPLD_DATA_LCDBUSY	BIT(7)
};

static void sead3_lcd_cpld_wait(struct udevice *dev)
{
	struct sead3_lcd_priv *priv = dev_get_priv(dev);
	uint status;

	do {
		regmap_read(priv->regmap, priv->offset + REG_CPLD_STAT, &status);
	} while (status & REG_CPLD_STAT_BUSY);
}

static void sead3_lcd_wait(struct udevice *dev)
{
	struct sead3_lcd_priv *priv = dev_get_priv(dev);
	uint status;

	sead3_lcd_cpld_wait(dev);

	do {
		regmap_read(priv->regmap, priv->offset + REG_INSN, &status);
		sead3_lcd_cpld_wait(dev);
		regmap_read(priv->regmap, priv->offset + REG_CPLD_DATA, &status);
	} while (status & REG_CPLD_DATA_LCDBUSY);
}

static void sead3_lcd_cmd(struct udevice *dev, unsigned char cmd)
{
	struct sead3_lcd_priv *priv = dev_get_priv(dev);

	sead3_lcd_wait(dev);
	regmap_write(priv->regmap, priv->offset + REG_INSN, cmd);
}

static void sead3_lcd_set(struct udevice *dev, int cmd)
{
	if (cmd & DISPLAY_CLEAR)
		sead3_lcd_cmd(dev, 0x01);

	if (cmd & DISPLAY_HOME)
		sead3_lcd_cmd(dev, 0x02);

	if (cmd & DISPLAY_NEWLINE)
		sead3_lcd_cmd(dev, 0xc0);
}

static void sead3_lcd_putc(struct udevice *dev, char c)
{
	struct sead3_lcd_priv *priv = dev_get_priv(dev);

	sead3_lcd_wait(dev);
	regmap_write(priv->regmap, priv->offset + REG_DATA, c);
}

static int sead3_lcd_bind(struct udevice *dev)
{
	struct lcd_display_uc_platdata *pdata = dev_get_uclass_platdata(dev);

	pdata->rows = 2;
	pdata->columns = 16;
	return 0;
}

static int sead3_lcd_probe(struct udevice *dev)
{
	struct sead3_lcd_priv *priv = dev_get_priv(dev);
	struct udevice *parent = dev_get_parent(dev);
	int err;

	priv->regmap = syscon_get_regmap(parent);
	if (IS_ERR(priv->regmap)) {
		err = PTR_ERR(priv->regmap);
		error("unable to find regmap: %d\n", err);
		return err;
	}

	priv->offset = dev_read_u32_default(dev, "offset", 0);
	return 0;
}

static const struct lcd_display_ops sead3_lcd_ops = {
	.set	= sead3_lcd_set,
	.putc	= sead3_lcd_putc,
};

static const struct udevice_id sead3_lcd_ids[] = {
	{ .compatible = "mti,sead3-lcd" },
	{ }
};

U_BOOT_DRIVER(sead_3_lcd) = {
	.id			= UCLASS_LCD,
	.name			= "sead3-lcd",
	.of_match		= sead3_lcd_ids,
	.bind			= sead3_lcd_bind,
	.probe			= sead3_lcd_probe,
	.ops			= &sead3_lcd_ops,
	.priv_auto_alloc_size	= sizeof(struct sead3_lcd_priv),
};
