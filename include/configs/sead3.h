/*
 * Copyright (C) 2015 Imagination Technologies
 *
 * SPDX-License-Identifier:	GPL-2.0
 */

#ifndef _SEAD3_CONFIG_H
#define _SEAD3_CONFIG_H

#include <asm/addrspace.h>

/*
 * System configuration
 */
#define CONFIG_BAUDRATE			115200


/*
 * CPU Configuration
 */
#define CONFIG_SYS_MHZ			50	/* arbitrary value */
#define CONFIG_SYS_MIPS_TIMER_FREQ	(CONFIG_SYS_MHZ * 1000000)

/*
 * Memory map
 */
#define CONFIG_SYS_MONITOR_BASE		CONFIG_SYS_TEXT_BASE

#define CONFIG_SYS_SDRAM_BASE		0x0

#define CONFIG_SYS_INIT_SP_OFFSET	0x400000

#define CONFIG_SYS_LOAD_ADDR		CKSEG0ADDR(0x01000000)
#define CONFIG_SYS_MEMTEST_START	CKSEG0ADDR(0x00100000)
#define CONFIG_SYS_MEMTEST_END		CKSEG0ADDR(0x00800000)

#define CONFIG_SYS_MALLOC_LEN		(4 * 1024 * 1024)
#define CONFIG_SYS_BOOTPARAMS_LEN	(128 * 1024)
#define CONFIG_SYS_BOOTM_LEN		(64 * 1024 * 1024)

/*
 * Console configuration
 */
#define CONFIG_SYS_CBSIZE		256
#define CONFIG_SYS_PBSIZE		(CONFIG_SYS_CBSIZE + \
					 sizeof(CONFIG_SYS_PROMPT) + 16)
#define CONFIG_SYS_MAXARGS		16

#define CONFIG_AUTO_COMPLETE
#define CONFIG_CMDLINE_EDITING

/*
 * Serial driver
 */
#define CONFIG_SYS_NS16550
#define CONFIG_SYS_CONSOLE_IS_IN_ENV
#define CONFIG_CONSOLE_MUX
#define CONFIG_CONS_INDEX		1
#define CONFIG_EXTRA_ENV_SETTINGS	\
	"stdin=eserial0,eserial1\0"	\
	"stdout=eserial0,eserial1\0"	\
	"stderr=eserial0,eserial1\0"

/*
 * Flash
 */
#define CONFIG_SYS_FLASH_BASE		(CKSEG1 | 0x1c000000)
#define CONFIG_SYS_MAX_FLASH_BANKS	1
#define CONFIG_SYS_MAX_FLASH_SECT	128
#define CONFIG_SYS_FLASH_CFI
#define CONFIG_FLASH_CFI_DRIVER
#define CONFIG_SYS_FLASH_USE_BUFFER_WRITE

/*
 * Environment
 */
#define CONFIG_ENV_IS_IN_FLASH
#define CONFIG_ENV_SECT_SIZE		0x40000
#define CONFIG_ENV_SIZE			CONFIG_ENV_SECT_SIZE
#define CONFIG_ENV_ADDR \
	(CONFIG_SYS_FLASH_BASE + (32 << 20) - CONFIG_ENV_SIZE)

/*
 * Ethernet
 */
#define CONFIG_SMC911X
#define CONFIG_SMC911X_32_BIT
#define CONFIG_SMC911X_BASE		CKSEG1ADDR(0x1f010000)
#define CONFIG_MII
#define CONFIG_PHYLIB
#define CONFIG_PHY_SMSC

#define CONFIG_UPDATE_TFTP

/* USB */
#define CONFIG_USB_MAX_CONTROLLER_COUNT	1
#define CONFIG_EHCI_IS_TDI


#define CONFIG_SYS_64BIT_LBA		1
#define CONFIG_LBA48			1

/*
 * Kernel arguments
 */
#define CONFIG_MEMSIZE_IN_BYTES


/*
 * Commands
 */
#define CONFIG_CMD_COP
#define CONFIG_CMD_DHCP
#define CONFIG_CMD_DISPLAY
#define CONFIG_CMD_EXT4
#define CONFIG_CMD_FAT
#define CONFIG_CMD_MEMTEST
#define CONFIG_CMD_PART
#define CONFIG_CMD_USB

#define CONFIG_SYS_LONGHELP		/* verbose help, undef to save memory */

#define CONFIG_DOS_PARTITION
#define CONFIG_EFI_PARTITION
#define CONFIG_PARTITION_UUIDS

#endif /* _SEAD3_CONFIG_H */
