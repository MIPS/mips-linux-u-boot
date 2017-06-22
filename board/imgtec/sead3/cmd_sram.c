/*
 * Copyright (C) 2015 Imagination Technologies
 *
 * SPDX-License-Identifier:	GPL-2.0
 */

#include <common.h>
#include <command.h>
#include <errno.h>
#include <inttypes.h>

#include <asm/io.h>
#include <asm/mipsregs.h>

#include "sram.h"

static const struct sram {
	const char	*name;
	u32		*cfg;
	u32		*mask;
	enum sram_type	type;
} srams[] = {
	{
		"ISRAM0",
		(u32 *)CKSEG1ADDR(0x1e000000),
		(u32 *)CKSEG1ADDR(0x1e000010),
		ISRAM,
	},
	{
		"ISRAM1",
		(u32 *)CKSEG1ADDR(0x1e000004),
		(u32 *)CKSEG1ADDR(0x1e000014),
		ISRAM,
	},
	{
		"DSRAM0",
		(u32 *)CKSEG1ADDR(0x1e000020),
		(u32 *)CKSEG1ADDR(0x1e000030),
		DSRAM,
	},
	{
		"DSRAM1",
		(u32 *)CKSEG1ADDR(0x1e000024),
		(u32 *)CKSEG1ADDR(0x1e000034),
		DSRAM,
	}
};

size_t sram_size(const struct sram *sram)
{
	const u32 cfg_tests[] = {
		0xf0000000,
		0x00000000,
	};
	u32 cfg, mask;
	unsigned i;

	cfg = __raw_readl(sram->cfg);
	mask = __raw_readl(sram->mask);

	/* check that cfg is writeable */
	for (i = 0; i < ARRAY_SIZE(cfg_tests); i++) {
		__raw_writel(cfg_tests[i], sram->cfg);
		if (__raw_readl(sram->cfg) != cfg_tests[i])
			return 0;
	}

	/* check that mask is not writeable */
	__raw_writel(0, sram->mask);
	if (__raw_readl(sram->mask) != mask)
		return 0;

	/* restore cfg */
	__raw_writel(cfg, sram->cfg);

	/* calculate SRAM size */
	return ~mask + 1;
}

int write_one_sram(const struct sram *sram, void *dst, void *src, size_t sz)
{
	u32 cfg, sram_sz, ecc;
	void *base, *start, *end, *src_start;

	if (sram->type != ISRAM)
		return -ENOMEM;

	cfg = __raw_readl(sram->cfg);
	if (!(cfg & SRAM_CFG_E))
		return -ENOMEM;

	sram_sz = sram_size(sram);
	if (!sram_sz)
		return -ENOMEM;

	base = (void *)CKSEG0ADDR(cfg & SRAM_CFG_PHYS);
	if ((dst + sz) < base)
		return -ENOMEM;
	if (dst >= (base + sram_sz))
		return -ENOMEM;

	start = max_t(void *, base, dst);
	src_start = src + (start - base);
	end = min_t(void *, base + sram_sz, dst + sz);

	printf("Loading %s [%p-%p <- %p]\n", sram->name, start, end, src_start);

	ecc = read_c0_ecc();

	asm volatile(
		".set	push\n"
		".set	noreorder\n"
		"	mtc0	%3, $26\n"
		"	sync\n"
		"	ehb\n"
		".set	push\n"
		".set	noat\n"
		"1:	lw	$1, 0(%1)\n"
		"	sw	$1, 0(%0)\n"
		".set	pop\n"
		"	addiu	%0, %0, 4\n"
		"	blt	%0, %4, 1b\n"
		"	 addiu	%1, %1, 4\n"
		"	sync\n"
		"	ehb\n"
		"	mtc0	%2, $26\n"
		"	sync\n"
		"	ehb\n"
		".set	pop\n"
		: "+r"(start),
		  "+r"(src_start)
		: "r"(ecc),
		  "r"(ecc | (1 << 28)),
		  "r"(end)
		: "$1");

	return 0;
}

int write_sram(void *dst, void *src, size_t sz)
{
	int i, err, overall_err = -ENOMEM;

	for (i = 0; i < ARRAY_SIZE(srams); i++) {
		err = write_one_sram(&srams[i], dst, src, sz);

		if (!err) {
			overall_err = 0;
			continue;
		}

		if (err != -ENOMEM)
			return err;
	}

	return overall_err;
}

int move_sram(enum sram_type type, phys_addr_t base)
{
	u32 cfg, mask;
	int i;

	for (i = 0; i < ARRAY_SIZE(srams); i++) {
		if (srams[i].type != type)
			continue;

		mask = __raw_readl(srams[i].mask);
		if (!mask)
			continue;

		cfg = base & mask;
		cfg |= SRAM_CFG_E;
		__raw_writel(cfg, srams[i].cfg);
		return 0;
	}

	return -ENOMEM;
}

void display_sram(int flags)
{
	u32 cfg, sz;
	int i;

	for (i = 0; i < ARRAY_SIZE(srams); i++) {
		sz = sram_size(&srams[i]);
		if (!sz && !(flags & DISP_ABSENT))
			continue;

		printf("%s: ", srams[i].name);

		if (!sz) {
			printf("not present\n");
			continue;
		}
		print_size(sz, "");

		cfg = __raw_readl(srams[i].cfg);
		if (!(cfg & SRAM_CFG_E)) {
			printf(" disabled\n");
			continue;
		}

		printf(" at 0x%08" PRIx32 "%s\n",
		       cfg & SRAM_CFG_PHYS,
		       (cfg & SRAM_CFG_R) ? ", redirect" : "");
	}
}

static void read_sram(const struct sram *sram, void *dest, size_t sz)
{
	uint32_t *insn, *dest_u32;
	uint32_t (*fn)(void);
	unsigned i, off;
	size_t bytes;
	void *base;

	base = (void *)CKSEG0ADDR(__raw_readl(sram->cfg) & SRAM_CFG_PHYS);

	bytes = sram_size(sram);
	if (sz)
		bytes = min_t(size_t, bytes, sz);

	if (sram->type != ISRAM) {
		memcpy(dest, base, bytes);
		return;
	}

	insn = base - 0x8;
	fn = (void *)insn;
	dest_u32 = dest;

	for (i = 0; i < DIV_ROUND_UP(bytes, 4); i++) {
		off = base + (i * 4) - (void *)insn;
		insn[0] = 0xec080000 | (2 << 21) | (off >> 2);
		insn[1] = 0xd8000000 | (31 << 16);
		flush_cache((ulong)insn, 8);
		dest_u32[i] = fn();
	}
}

int do_sram(cmd_tbl_t *cmdtp, int flag, int argc, char * const argv[])
{
	void *dest;
	size_t sz;

	if (argc < 2)
		return 1;

	if (!strcmp(argv[1], "info")) {
		display_sram(DISP_ABSENT);
		return 0;
	}

	if (!strcmp(argv[1], "read")) {
		if (argc < 3)
			return 1;

		dest = (void *)simple_strtoul(argv[2], NULL, 16);

		if (argc >= 4)
			sz = simple_strtoul(argv[3], NULL, 16);
		else
			sz = 0;

		read_sram(&srams[0], dest, sz);
		return 0;
	}

	return 1;
}

U_BOOT_CMD(
	sram,      4,      0,      do_sram,
	"Set up SEAD3 SRAMs",
	"[info|move|read]\n"
	"\t- load ELF image at [address] to the appropriate memories"
);
