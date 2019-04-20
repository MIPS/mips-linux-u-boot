/* SPDX-License-Identifier: GPL-2.0 */
/*
 * Copyright (C) 2016 Imagination Technologies
 */

#include <config.h>
#include <pci.h>

#include <asm/addrspace.h>
#include <asm/asm.h>
#include <asm/mipsregs.h>
#include <asm/regdef.h>

#include "boston-regs.h"

#define DEV_CFG	(1 << XLNX_PCIE_CFG_BUS_SHIFT)

#if defined(__MIPSEB__)
#define swap_64(reg) \
			dsbh reg,reg; \
			dshd reg, reg;

#define swap_32(reg) \
			wsbh reg,reg; \
			rotr reg, reg, 16;
#else
#define	swap_64(reg)	nop	
#define swap_32(reg)	nop
#endif

.data

msg_ddr_cal:	.ascii "DDR Cal "
msg_ddr_ok:	.ascii "DDR OK  "

.text

LEAF(lowlevel_init)
	move	s0, ra

	/* Check whether we have a sleipnir attached */
	bal	detect_sleipnir
	beqz	v0, 1f

	/* Enable PCIe0 bridge */
	li	t1, XLNX_PCIE_RPSC_BRIDGE_EN
	swap_32(t1)
	sw	t1, XLNX_PCIE_RPSC(v1)

	/* Set PCIe0 BAR */
	li	t1, 0x0100
	swap_32(t1)
	sw	t1, PCI_BASE_ADDRESS_2(v1)

	/* Enable PCIe0 bus mastering & memory space */
	li	t1, PCI_COMMAND_MASTER | PCI_COMMAND_MEMORY
	swap_32(t1)
	sw	t1, PCI_COMMAND(v1)

	/* Set BAR */
	li	t1, 0x10000000
	swap_32(t1)
	sw	t1, (DEV_CFG + PCI_BASE_ADDRESS_0)(v1)

	/* Enable bus mastering & memory space */
	li	t1, PCI_COMMAND_MASTER | PCI_COMMAND_MEMORY
	swap_32(t1)
	sw	t1, (DEV_CFG + PCI_COMMAND)(v1)

1:	PTR_LA	a0, msg_ddr_cal
	bal	lowlevel_display

	PTR_LI	t0, BOSTON_PLAT_DDR3STAT
1:	lw	t1, 0(t0)
	swap_32(t1)
	andi	t1, t1, BOSTON_PLAT_DDR3STAT_CALIB
	beqz	t1, 1b

	PTR_LA	a0, msg_ddr_ok
	bal	lowlevel_display

	move	v0, zero
	jr	s0
	END(lowlevel_init)

LEAF(detect_sleipnir)
	/* Check if we have a PCIe0 controller */
	li	t0, BOSTON_PLAT_BUILDCFG0
	lw	t0, 0(t0)
	swap_32(t0)
	andi	t0, t0, BOSTON_PLAT_BUILDCFG0_PCIE0
	beqz	t0, 1f

	/* Check if PCIe0 link is up */
	li      v1, BOSTON_PCIE0_BASE
	lw	t0, XLNX_PCIE_PSCR(v1)
	swap_32(t0)
	andi	t0, t0, XLNX_PCIE_PSCR_LINK_UP
	beqz	t0, 1f

	/* Setup subordinate & secondary to allow downstream config access */
	li	t1, (1 << ((PCI_SUBORDINATE_BUS - PCI_PRIMARY_BUS) * 8)) | \
		    (1 << ((PCI_SECONDARY_BUS - PCI_PRIMARY_BUS) * 8))
	swap_32(t1)
	sw	t1, PCI_PRIMARY_BUS(v1)

	/* Check whether a sleipnir is attached to PCIe0 */
	lw	t0, (DEV_CFG + PCI_VENDOR_ID)(v1)
	swap_32(t0)
	li	t1, (PCI_DEVICE_ID_MIPS_SLEIPNIR << 16) | PCI_VENDOR_ID_MIPS
	bne	t0, t1, 1f

	/* Return 1; we found a sleipnir */
	li	v0, 1
	jr	ra

	/* Return 0; no sleipnir */
1:	move	v0, zero
	jr	ra
	END(detect_sleipnir)

LEAF(lowlevel_display)
	.set	push
	.set	noat
	PTR_LI	AT, BOSTON_LCD_BASE
#ifdef CONFIG_64BIT
	ld	k1, 0(a0)
	sd	k1, 0(AT)
#else
	lw	k1, 0(a0)
	sw	k1, 0(AT)
	lw	k1, 4(a0)
	sw	k1, 4(AT)
#endif
	.set	pop
	jr	ra
	END(lowlevel_display)
