/* SPDX-License-Identifier: GPL-2.0 */
/*
 * Copyright (C) 2016 Imagination Technologies
 */

#ifndef __CONFIGS_BOSTON_H__
#define __CONFIGS_BOSTON_H__

/*
 * General board configuration
 */
#define CONFIG_SYS_BOOTM_LEN		(64 * 1024 * 1024)

/*
 * CPU
 */
#define CONFIG_SYS_MIPS_TIMER_FREQ	30000000

/*
 * PCI
 */
#define CONFIG_SYS_PCI_64BIT

/*
 * Memory map
 */
#ifdef CONFIG_64BIT
# define CONFIG_SYS_SDRAM_BASE		0xffffffff80000000
#else
# define CONFIG_SYS_SDRAM_BASE		0x80000000
#endif

#define CONFIG_SYS_INIT_SP_OFFSET	0x400000

#define CONFIG_SYS_MONITOR_BASE		CONFIG_SYS_TEXT_BASE

#define CONFIG_SYS_LOAD_ADDR		(CONFIG_SYS_SDRAM_BASE + 0x08000000)

#define CONFIG_SYS_MEMTEST_START	(CONFIG_SYS_SDRAM_BASE + 0)
#define CONFIG_SYS_MEMTEST_END		(CONFIG_SYS_SDRAM_BASE + 0x10000000)

#define CONFIG_SYS_MALLOC_LEN		(256 * 1024)

/*
 * Console
 */

/*
 * Flash
 */
#define CONFIG_SYS_MAX_FLASH_BANKS_DETECT	1
#define CONFIG_SYS_MAX_FLASH_SECT		1024

/*
 * Environment
 */
#define CONFIG_ENV_SECT_SIZE		0x40000
#define CONFIG_ENV_SIZE			CONFIG_ENV_SECT_SIZE
#ifdef CONFIG_64BIT
# define CONFIG_ENV_ADDR \
	(0xffffffffb8000000 + (128 << 20) - CONFIG_ENV_SIZE)
#else
# define CONFIG_ENV_ADDR \
	(0xb8000000 + (128 << 20) - CONFIG_ENV_SIZE)
#endif

/*
 * Static env callback for the IOCU.
 */
#define CONFIG_ENV_CALLBACK_LIST_STATIC \
	"io" ENV_DOT_ESCAPE ".coherent:io_coherent,"

/*
 * SATA
 */
#define CONFIG_SYS_SCSI_MAX_SCSI_ID	2
#define CONFIG_SYS_SCSI_MAX_DEVICE	2
#define CONFIG_SYS_SCSI_MAX_LUN		1
#define CONFIG_SCSI_DEV_LIST \
	{ PCI_VENDOR_ID_INTEL, PCI_DEVICE_ID_INTEL_TCF_SATA }

/*
 * Miscellaneous IOCU init.
 */
#define CONFIG_MISC_INIT_F

#endif /* __CONFIGS_BOSTON_H__ */
