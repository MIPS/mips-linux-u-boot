/*
 * Copyright (C) 2017 MIPS Technologies LLC
 * Author: Paul Burton <paul.burton@imgtec.com>
 *
 * SPDX-License-Identifier:	GPL-2.0+
 */

#include <common.h>
#include <command.h>
#include <asm/mipsregs.h>

struct cop_reg {
	unsigned reg, sel;
	char *name;
	bool (*present)(struct cop_reg *reg);
	unsigned check_reg, check_bit;
};

static int read_cop_register(unsigned cop, unsigned reg, unsigned sel,
			     unsigned long *val, unsigned sz)
{
	unsigned long (*fn)(void);

#ifdef __nanomips__
	unsigned short insns[] = {
		0x2080, 0x0030,		/* mfc0 va0, reg, sel */
		0xdbe0,			/* jrc ra */
	};

	if (cop != 0 || sz != 4)
		return 1;

	insns[0] |= reg;
	insns[1] |= sel << 11;
#else
	unsigned long insns[] = {
		0x40020000,		/* mfcX v0, reg, sel */
		0x03e00009,		/* jr ra */
		0x00000000,		/*  nop */
	};

	switch (sz) {
	case 4:
		insns[0] |= 0 << 21;
		break;
	case 8:
		insns[0] |= 1 << 21;
		break;
	}

	insns[0] |= cop << 26;
	insns[0] |= reg << 11;
	insns[0] |= sel;
#endif

	flush_cache((unsigned long)insns, sizeof(insns));
	fn = (void *)insns;
	*val = fn();
	return 0;
}

static int write_cop_register(unsigned cop, unsigned reg, unsigned sel,
			      unsigned long val, unsigned sz)
{
	void (*fn)(unsigned long val);

#ifdef __nanomips__
	unsigned short insns[] = {
		0x2080, 0x0070,		/* mtc0 a0, reg, sel */
		0xdbe0,			/* jrc ra */
	};

	if (cop != 0 || sz != 4)
		return 1;

	insns[0] |= reg;
	insns[1] |= sel << 11;
#else
	unsigned long insns[] = {
		0x40040000,		/* mtcX a0, reg, sel */
		0x03e00009,		/* jr ra */
		0x00000000,		/*  nop */
	};

	switch (sz) {
	case 4:
		insns[0] |= 4 << 21;
		break;
	case 8:
		insns[0] |= 5 << 21;
		break;
	}

	insns[0] |= cop << 26;
	insns[0] |= reg << 11;
	insns[0] |= sel;
#endif

	flush_cache((unsigned long)insns, sizeof(insns));
	fn = (void *)insns;
	fn(val);
	return 0;
}

static bool mipsr2_or_higher(struct cop_reg *reg)
{
	return read_c0_config() & MIPS_CONF_AR;
}

static bool _config_present(unsigned cfg_sel)
{
	unsigned long val;
	unsigned sel;
	int err;

	for (sel = 1; sel < cfg_sel; sel++) {
		err = read_cop_register(0, 16, sel, &val, sizeof(u32));
		if (err || !(val & MIPS_CONF_M))
			return false;
	}

	return true;
}

static bool config_present(struct cop_reg *reg)
{
	return _config_present(reg->sel);
}

static bool config_bit_set(struct cop_reg *reg)
{
	unsigned long val;
	int err;

	if (!_config_present(reg->check_reg))
		return false;

	err = read_cop_register(0, 16, reg->check_reg, &val, sizeof(u32));
	if (err)
		return false;

	return val & reg->check_bit;
}

static struct cop_reg cop0_regs[] = {
	{  3, 1, "GlobalNumber", config_bit_set, 5, 1 << 7 },
	{  4, 2, "UserLocal", config_bit_set, 3, 1 << 13 },
	{  5, 2, "SegCtl0", config_bit_set, 3, 1 << 25 },
	{  5, 3, "SegCtl1", config_bit_set, 3, 1 << 25 },
	{  5, 4, "SegCtl2", config_bit_set, 3, 1 << 25 },
	{  5, 5, "PWBase", config_bit_set, 3, 1 << 24 },
	{  5, 6, "PWField", config_bit_set, 3, 1 << 24 },
	{  5, 7, "PWSize", config_bit_set, 3, 1 << 24 },
	{  8, 0, "BadVAddr" },
	{  8, 1, "BadInstr", config_bit_set, 3, 1 << 26 },
	{  8, 2, "BadInstrP", config_bit_set, 3, 1 << 27 },
#ifdef __nanomips__
	{  8, 3, "BadInstrX", config_bit_set, 3, 1 << 26 },
#endif
	{  9, 0, "Count" },
	{ 11, 0, "Compare" },
	{ 12, 0, "Status" },
	{ 13, 0, "Cause" },
	{ 14, 0, "EPC" },
	{ 14, 2, "NestedEPC", config_bit_set, 5, 1 << 0 },
	{ 15, 0, "PRId" },
	{ 15, 1, "EBase", mipsr2_or_higher },
	{ 15, 3, "CMGCRBase", config_bit_set, 3, 1 << 29 },
	{ 16, 0, "Config" },
	{ 16, 1, "Config1", config_present },
	{ 16, 2, "Config2", config_present },
	{ 16, 3, "Config3", config_present },
	{ 16, 4, "Config4", config_present },
	{ 16, 5, "Config5", config_present },
	{ 17, 0, "LLAddr", config_bit_set, 5, 1 << 4 },
	{ 30, 0, "ErrorEPC" },
	{ 31, 2, "KScratch1", config_bit_set, 4, 1 << 18 },
	{ 31, 3, "KScratch2", config_bit_set, 4, 1 << 19 },
	{ 31, 4, "KScratch3", config_bit_set, 4, 1 << 20 },
	{ 31, 5, "KScratch4", config_bit_set, 4, 1 << 21 },
	{ 31, 6, "KScratch5", config_bit_set, 4, 1 << 22 },
	{ 31, 7, "KScratch6", config_bit_set, 4, 1 << 23 },
	{ 0, 0, NULL },
};

static struct cop_reg cop1_regs[] = {
	{ 0, 0, NULL },
};

static struct cop_reg cop2_regs[] = {
	{ 0, 0, NULL },
};

static struct cop_reg *cop_regs[] = {
	cop0_regs,
	cop1_regs,
	cop2_regs,
};

static int do_cop_overview(unsigned cop, unsigned size)
{
	struct cop_reg *reg;
	unsigned long val;
	int err;

	for (reg = cop_regs[cop]; reg->name; reg++) {
		if (reg->present && !reg->present(reg))
			continue;

		err = read_cop_register(cop, reg->reg, reg->sel, &val, size);
		if (err)
			return err;

		printf("%02u.%u %15s: 0x%0*lx\n",
		       reg->reg, reg->sel, reg->name, size * 2, val);
	}

	return 0;
}

static int do_cop(cmd_tbl_t *cmdtp, int flag, int argc, char * const argv[])
{
	char *str_cop, *str_reg, *str_sel, *str_val;
	unsigned long cop, reg, sel, val;
	unsigned size;
	int err;

	if (argc < 2)
		return CMD_RET_USAGE;

	if ((size = cmd_get_data_size(argv[0], sizeof(ulong))) < 0)
		return CMD_RET_USAGE;

	switch (size) {
	case 4:
	case 8:
		break;

	default:
		return CMD_RET_USAGE;
	}

	str_cop = argv[1];
	err = strict_strtoul(str_cop, 10, &cop);
	if (err || (cop > 2))
		return CMD_RET_USAGE;

	if (argc == 2)
		return do_cop_overview(cop, size);

	str_reg = argv[2];
	str_sel = strchr(str_reg, '.');
	if (str_sel) {
		*str_sel++ = '\0';
		err = strict_strtoul(str_sel, 10, &sel);
		if (err)
			return CMD_RET_USAGE;
	} else {
		sel = 0;
	}

	err = strict_strtoul(str_reg, 10, &reg);
	if (err)
		return CMD_RET_USAGE;

	if (argc == 3) {
		/* Read coprocessor register */
		err = read_cop_register(cop, reg, sel, &val, size);
		if (err)
			return err;

		printf("0x%0*lx\n", size * 2, val);
		return 0;
	}

	/* Write coprocessor register */
	str_val = argv[3];
	err = strict_strtoul(str_val, 16, &val);
	if (err)
		return CMD_RET_USAGE;

	return write_cop_register(cop, reg, sel, val, size);
}

U_BOOT_CMD(
	cop,	4,	1,	do_cop,
	"access coprocessor registers",
	"[.l, .q] coprocessor [register[.select]] [value]"
);
