/*
 * Copyright (C) 2016 Imagination Technologies
 *
 * SPDX-License-Identifier:	GPL-2.0
 */

#include <common.h>
#include <cpu.h>
#include <dm/device.h>
#include <asm/mipsregs.h>

static int cpu_mips_bind(struct udevice *dev)
{
	struct cpu_platdata *plat = dev_get_parent_platdata(dev);

	plat->device_id = read_c0_prid();

	return 0;
}

static const char *cpu_name_mti_img(u32 prid)
{
	switch (prid & PRID_PROCESSOR) {
	case PRID_PROCESSOR_MTI_INTERAPTIVUP:
	case PRID_PROCESSOR_MTI_INTERAPTIVMP:
		return "interAptiv";
	case PRID_PROCESSOR_MTI_PROAPTIVUP:
	case PRID_PROCESSOR_MTI_PROAPTIVMP:
		return "proAptiv";
	case PRID_PROCESSOR_IMG_P6600:
		return "P6600";
	case PRID_PROCESSOR_IMG_M5150:
		return "M5150";
	case PRID_PROCESSOR_IMG_P5600:
		return "P5600";
	case PRID_PROCESSOR_IMG_I6400:
		return "I6400";
	case PRID_PROCESSOR_IMG_M6250:
		return "M6250";
	case PRID_PROCESSOR_IMG_I6500:
		return "I6500";
	}

	return NULL;
}

ulong __weak cpu_mips_rtl_changelist(struct udevice *dev)
{
	return 0;
}

void __weak cpu_mips_rtl_misc(struct udevice *dev, char *buf, int size)
{
}

static int cpu_mips_get_desc(struct udevice *dev, char *buf, int size)
{
	struct cpu_platdata *plat = dev_get_parent_platdata(dev);
	const char *name = NULL;
	ulong cl;
	int idx;

	switch (plat->device_id & PRID_COMPANY) {
	case PRID_COMPANY_MTI_IMG:
		name = cpu_name_mti_img(plat->device_id);
		break;
	}

	name = name ?: "Unknown";
	idx = snprintf(buf, size, "%s", name);

	cl = cpu_mips_rtl_changelist(dev);
	if (cl && (idx < size))
		idx += snprintf(&buf[idx], size - idx, " cl%lu", cl);

	cpu_mips_rtl_misc(dev, &buf[idx], size - idx);

	return 0;
}

static int cpu_mips_get_vendor(struct udevice *dev, char *buf, int size)
{
	strncpy(buf, "Imagination Technologies", size);
	buf[size - 1] = 0;

	return 0;
}

static int cpu_mips_get_info(struct udevice *dev, struct cpu_info *info)
{
	memset(info, 0, sizeof(*info));

	info->features |= 1 << CPU_FEAT_L1_CACHE;
	info->features |= 1 << CPU_FEAT_MMU;
	info->features |= 1 << CPU_FEAT_DEVICE_ID;

	return 0;
}

static const struct cpu_ops cpu_mips_ops = {
	.get_desc	= cpu_mips_get_desc,
	.get_vendor	= cpu_mips_get_vendor,
	.get_info	= cpu_mips_get_info,
};

static const struct udevice_id cpu_mips_ids[] = {
	{ .compatible = "cpu-mips" },
	{ }
};

U_BOOT_DRIVER(cpu_mips_drv) = {
	.name		= "cpu_mips",
	.id		= UCLASS_CPU,
	.of_match	= cpu_mips_ids,
	.bind		= cpu_mips_bind,
	.ops		= &cpu_mips_ops,
};
