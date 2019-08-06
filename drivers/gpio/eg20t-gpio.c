/*
 * Copyright (C) 2016 Imagination Technologies
 *
 * SPDX-License-Identifier:	GPL-2.0
 */

#include <common.h>
#include <dm.h>
#include <errno.h>
#include <pci.h>
#include <asm/io.h>
#include <asm/gpio.h>

enum {
	REG_IEN		= 0x00,
	REG_ISTATUS	= 0x04,
	REG_IDISP	= 0x08,
	REG_ICLR	= 0x0c,
	REG_IMASK	= 0x10,
	REG_IMASKCLR	= 0x14,
	REG_PO		= 0x18,
	REG_PI		= 0x1c,
	REG_PM		= 0x20,
};

struct eg20t_gpio_priv {
	void *base;
};

static int eg20t_gpio_get_value(struct udevice *dev, unsigned int offset)
{
	struct eg20t_gpio_priv *priv = dev_get_priv(dev);
	uint32_t pm, pval;

	pm = readl(priv->base + REG_PM);
	if ((pm >> offset) & 0x1)
		pval = readl(priv->base + REG_PO);
	else
		pval = readl(priv->base + REG_PI);

	return (pval >> offset) & 0x1;
}

static int eg20t_gpio_set_value(struct udevice *dev, unsigned int offset,
				int value)
{
	struct eg20t_gpio_priv *priv = dev_get_priv(dev);
	uint32_t po;

	po = readl(priv->base + REG_PO);
	if (value)
		po |= 1 << offset;
	else
		po &= ~(1 << offset);
	writel(po, priv->base + REG_PO);
	return 0;
}

static int eg20t_gpio_direction_input(struct udevice *dev, unsigned int offset)
{
	struct eg20t_gpio_priv *priv = dev_get_priv(dev);
	uint32_t pm;

	pm = readl(priv->base + REG_PM);
	pm &= ~(1 << offset);
	writel(pm, priv->base + REG_PM);
	return 0;
}

static int eg20t_gpio_direction_output(struct udevice *dev, unsigned int offset,
				       int value)
{
	struct eg20t_gpio_priv *priv = dev_get_priv(dev);
	uint32_t pm;

	pm = readl(priv->base + REG_PM);
	pm |= 1 << offset;
	writel(pm, priv->base + REG_PM);

	return eg20t_gpio_set_value(dev, offset, value);
}

static int eg20t_gpio_get_function(struct udevice *dev, unsigned int offset)
{
	struct eg20t_gpio_priv *priv = dev_get_priv(dev);
	uint32_t pm;

	pm = readl(priv->base + REG_PM);

	if ((pm >> offset) & 0x1)
		return GPIOF_OUTPUT;

	return GPIOF_INPUT;
}

static const struct dm_gpio_ops eg20t_gpio_ops = {
	.direction_input	= eg20t_gpio_direction_input,
	.direction_output	= eg20t_gpio_direction_output,
	.get_value		= eg20t_gpio_get_value,
	.set_value		= eg20t_gpio_set_value,
	.get_function		= eg20t_gpio_get_function,
};

static int eg20t_gpio_probe(struct udevice *dev)
{
	struct eg20t_gpio_priv *priv = dev_get_priv(dev);
	struct gpio_dev_priv *uc_priv = dev->uclass_priv;

	priv->base = dm_pci_map_bar(dev, PCI_BASE_ADDRESS_1, PCI_REGION_MEM);
	if (!priv->base) {
		debug("failed to map GPIO registers\n");
		return -EINVAL;
	}

	uc_priv->gpio_count = 12;
	uc_priv->bank_name = "eg20t";
	return 0;
}

static const struct udevice_id eg20t_gpio_ids[] = {
	{ .compatible = "intel,eg20t-gpio" },
	{ }
};

U_BOOT_DRIVER(eg20t_gpio) = {
	.name	= "eg20t-gpio",
	.id	= UCLASS_GPIO,
	.of_match = eg20t_gpio_ids,
	.probe	= eg20t_gpio_probe,
	.priv_auto_alloc_size = sizeof(struct eg20t_gpio_priv),
	.ops	= &eg20t_gpio_ops,
};

static struct pci_device_id eg20t_gpio_supported[] = {
	{ PCI_VENDOR_ID_INTEL, 0x8803 },
	{ },
};

U_BOOT_PCI_DEVICE(eg20t_gpio, eg20t_gpio_supported);
