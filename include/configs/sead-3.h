/*
 * SEAD-3 Board Configuration
 *
 * Copyright (c) 2017 Imagination Technologies Ltd.
 *
 * SPDX-License-Identifier:	GPL-2.0+
 */

#ifndef __CONFIGS_SEAD_3_H__
#define __CONFIGS_SEAD_3_H__

/*
 * CPU timer
 */
#define CONFIG_SYS_MIPS_TIMER_FREQ		(50 * 1000 * 1000 / 2)

/*
 * Memory map
 */
#define CONFIG_SYS_MONITOR_BASE			CONFIG_SYS_TEXT_BASE
#define CONFIG_SYS_SDRAM_BASE			0x80000000
#define CONFIG_SYS_LOAD_ADDR			(CONFIG_SYS_SDRAM_BASE + (1 << 20))
#define CONFIG_SYS_INIT_SP_OFFSET		(16 * 1024)
#define CONFIG_SYS_MALLOC_LEN			(512 * 1024)

/*
 * Console
 */
#define CONFIG_SYS_CBSIZE			256
#define CONFIG_SYS_PBSIZE			(CONFIG_SYS_CBSIZE + sizeof(CONFIG_SYS_PROMPT) + 16)
#define CONFIG_SYS_MAXARGS			16
#define CONFIG_SYS_LONGHELP

/*
 * System flash
 */
#define CONFIG_SYS_FLASH_BASE			0xbc000000
#define CONFIG_FLASH_CFI_DRIVER
#define CONFIG_SYS_FLASH_CFI
#define CONFIG_SYS_FLASH_PROTECTION
#define CONFIG_SYS_FLASH_USE_BUFFER_WRITE
#define CONFIG_SYS_MAX_FLASH_BANKS_DETECT	1

#endif /* __CONFIGS_SEAD_3_H__ */
