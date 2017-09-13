/*
 * Copyright (C) 2015 Imagination Technologies
 *
 * SPDX-License-Identifier:	GPL-2.0
 */

#include <common.h>
#include <command.h>
#include <elf.h>
#include <errno.h>

#include "sram.h"

static void config_srams(const Elf32_Ehdr *ehdr)
{
	const Elf32_Shdr *shdr;
	phys_addr_t inst_base = 0;
	phys_addr_t data_base = 0;
	int i;

	shdr = (Elf32_Shdr *)((void *)ehdr + ehdr->e_shoff);

	for (i = 0; i < ehdr->e_shnum; i++, shdr++) {
		if (!(shdr->sh_flags & SHF_ALLOC))
			continue;

		if (shdr->sh_flags & SHF_EXECINSTR) {
			if (!inst_base)
				inst_base = shdr->sh_addr;
			else
				inst_base = min_t(phys_addr_t, inst_base, shdr->sh_addr);
		} else {
			if (!data_base)
				data_base = shdr->sh_addr;
			else
				data_base = min_t(phys_addr_t, data_base, shdr->sh_addr);
		}
	}

	if (inst_base)
		move_sram(ISRAM, CPHYSADDR(inst_base));

	if (data_base)
		move_sram(DSRAM, CPHYSADDR(data_base));

	display_sram(DISP_ABSENT);
}

static bool valid_elf(const Elf32_Ehdr *ehdr)
{
	if (!IS_ELF(*ehdr))
		return false;

	if (ehdr->e_type != ET_EXEC)
		return false;

	return true;
}

static int do_loadelf(cmd_tbl_t *cmdtp, int flag, int argc, char * const argv[])
{
	const Elf32_Ehdr *ehdr;
	const Elf32_Phdr *phdr;
	void *src, *dst;
	int i, err;
	bool auto_sram = false;

	if (argc > 1 && !strcmp(argv[1], "-auto-sram")) {
		auto_sram = true;
		argc--;
		argv++;
	}

	if (argc < 2)
		ehdr = (const void *)load_addr;
	else
		ehdr = (const void *)simple_strtoul(argv[1], NULL, 16);

	if (!valid_elf(ehdr)) {
		printf("No valid ELF at 0x%p\n", ehdr);
		return 1;
	}

	if (auto_sram)
		config_srams(ehdr);

	phdr = (Elf32_Phdr *)((void *)ehdr + ehdr->e_phoff);

	for (i = 0; i < ehdr->e_phnum; i++, phdr++) {
		src = (void *)ehdr + phdr->p_offset;
		dst = (void *)(uintptr_t)phdr->p_paddr;

		if (phdr->p_filesz) {
			memcpy(dst, src, phdr->p_filesz);
			err = write_sram(dst, src, phdr->p_filesz);
			if (err && (err != -ENOMEM)) {
				printf("Failed to write to SRAM\n");
				return err;
			}
		}

		if (phdr->p_filesz != phdr->p_memsz) {
			memset(dst + phdr->p_filesz, 0x00,
			       phdr->p_memsz - phdr->p_filesz);
		}

		flush_cache((unsigned long)dst, phdr->p_memsz);
	}

	setenv_addr("entryaddr", (void *)ehdr->e_entry);

	return 0;
}

U_BOOT_CMD(
	loadelf,      2,      0,      do_loadelf,
	"Load an ELF image in memory",
	"[-auto-sram] [address]\n"
	"\t- load ELF image at [address] to the appropriate memories"
);
