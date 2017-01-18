/*
 * Adapted from Linux v2.6.36 kernel: arch/powerpc/kernel/asm-offsets.c
 *
 * This program is used to generate definitions needed by
 * assembly language modules.
 *
 * We use the technique used in the OSF Mach kernel code:
 * generate asm statements containing #defines,
 * compile this file to assembler, and then extract the
 * #defines from the assembly-language output.
 *
 * SPDX-License-Identifier:	GPL-2.0+
 */

#include <common.h>
#include <spd.h>

#include <linux/kbuild.h>

int main(void)
{
	/* Round up to make sure size gives nice stack alignment */
	DEFINE(GENERATED_GBL_DATA_SIZE,
		(sizeof(struct global_data) + 15) & ~15);

	DEFINE(GENERATED_BD_INFO_SIZE,
		(sizeof(struct bd_info) + 15) & ~15);

	DEFINE(GD_SIZE, sizeof(struct global_data));

	DEFINE(GD_BD, offsetof(struct global_data, bd));
#ifdef CONFIG_SYS_MALLOC_F_LEN
	DEFINE(GD_MALLOC_BASE, offsetof(struct global_data, malloc_base));
#endif

	DEFINE(GD_RELOCADDR, offsetof(struct global_data, relocaddr));

	DEFINE(GD_RELOC_OFF, offsetof(struct global_data, reloc_off));

	DEFINE(GD_START_ADDR_SP, offsetof(struct global_data, start_addr_sp));

	return 0;
}

void spd_definitions(void)
{
	DEFINE(SPD_MEM_TYPE, offsetof(struct spd_eeprom_s, mem_type));
	DEFINE(SPD_NROW_ADDR, offsetof(struct spd_eeprom_s, nrow_addr));
	DEFINE(SPD_NCOL_ADDR, offsetof(struct spd_eeprom_s, ncol_addr));
	DEFINE(SPD_NROWS, offsetof(struct spd_eeprom_s, nrows));
	DEFINE(SPD_DATAW_LSB, offsetof(struct spd_eeprom_s, dataw_lsb));
	DEFINE(SPD_DATAW_MSB, offsetof(struct spd_eeprom_s, dataw_msb));
	DEFINE(SPD_CONFIG, offsetof(struct spd_eeprom_s, config));
	DEFINE(SPD_REFRESH, offsetof(struct spd_eeprom_s, refresh));
	DEFINE(SPD_NBANKS, offsetof(struct spd_eeprom_s, nbanks));
	DEFINE(SPD_CAS_LAT, offsetof(struct spd_eeprom_s, cas_lat));
	DEFINE(SPD_MOD_ATTR, offsetof(struct spd_eeprom_s, mod_attr));
	DEFINE(SPD_TRP, offsetof(struct spd_eeprom_s, trp));
	DEFINE(SPD_TRRD, offsetof(struct spd_eeprom_s, trrd));
	DEFINE(SPD_TRCD, offsetof(struct spd_eeprom_s, trcd));
	DEFINE(SPD_TRAS, offsetof(struct spd_eeprom_s, tras));
	DEFINE(SPD_TRC, offsetof(struct spd_eeprom_s, trc));
}
