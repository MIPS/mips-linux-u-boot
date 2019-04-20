/* SPDX-License-Identifier: GPL-2.0 */
/*
 * Copyright (C) 2016 Imagination Technologies
 */

#ifndef __BOARD_BOSTON_REGS_H__
#define __BOARD_BOSTON_REGS_H__

#include <asm/addrspace.h>

#define BOSTON_PCIE0_BASE		CKSEG1ADDR(0x10000000)
#define BOSTON_PLAT_BASE		CKSEG1ADDR(0x17ffd000)
#define BOSTON_LCD_BASE			CKSEG1ADDR(0x17fff000)

/*
 * Xilinx AXI Bridge for PCI Express Registers
 */
#define XLNX_PCIE_PSCR			0x144
#define  XLNX_PCIE_PSCR_LINK_UP		(0x1 << 11)
#define XLNX_PCIE_RPSC			0x148
#define  XLNX_PCIE_RPSC_BRIDGE_EN	0x1
#define XLNX_PCIE_CFG_BUS_SHIFT		20

/*
 * Platform Register Definitions
 */
#define BOSTON_PLAT_CORE_CL		(BOSTON_PLAT_BASE + 0x04)

#define BOSTON_PLAT_DDR3STAT		(BOSTON_PLAT_BASE + 0x14)
# define BOSTON_PLAT_DDR3STAT_CALIB	(1 << 2)

#define BOSTON_PLAT_BUILDCFG0		(BOSTON_PLAT_BASE + 0x34)
# define BOSTON_PLAT_BUILDCFG0_IOCU	(0x1 << 0)
# define BOSTON_PLAT_BUILDCFG0_PCIE0	(0x1 << 1)
# define BOSTON_PLAT_BUILDCFG0_PCIE1	(0x1 << 2)
# define BOSTON_PLAT_BUILDCFG0_PCIE2	(0x1 << 3)
# define BOSTON_PLAT_BUILDCFG0_CFG_LTR	(0xf << 4)
# define BOSTON_PLAT_BUILDCFG0_CFG_NUM	(0xff << 8)
# define BOSTON_PLAT_BUILDCFG0_DP	(0x1 << 24)
# define BOSTON_PLAT_BUILDCFG0_DP_MULT	(0xf << 28)

#define BOSTON_PLAT_DDRCONF0		(BOSTON_PLAT_BASE + 0x38)
# define BOSTON_PLAT_DDRCONF0_SIZE	(0xf << 0)

#define BOSTON_PLAT_NOCPCIE0ADDR	(BOSTON_PLAT_BASE + 0x3c)
#define BOSTON_PLAT_NOCPCIE1ADDR	(BOSTON_PLAT_BASE + 0x40)
#define BOSTON_PLAT_NOCPCIE2ADDR	(BOSTON_PLAT_BASE + 0x44)

#ifndef __ASSEMBLY__
extern bool detect_sleipnir(void);
#endif

#endif /* __BOARD_BOSTON_REGS_H__ */
