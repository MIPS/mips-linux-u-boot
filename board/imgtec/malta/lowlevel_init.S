/*
 * Copyright (C) 2013 Gabor Juhos <juhosg@openwrt.org>
 *
 * SPDX-License-Identifier:	GPL-2.0
 */

#include <asm-offsets.h>
#include <config.h>
#include <gt64120.h>
#include <intel_i2c.h>
#include <msc01.h>
#include <pci.h>
#include <spd.h>

#include <asm/addrspace.h>
#include <asm/asm.h>
#include <asm/regdef.h>
#include <asm/mipsregs.h>

#include "malta.h"

#ifdef CONFIG_SYS_BIG_ENDIAN
#define CPU_TO_GT32(_x)		((_x))
#else
#define CPU_TO_GT32(_x) (					\
	(((_x) & 0xff) << 24) | (((_x) & 0xff00) << 8) |	\
	(((_x) & 0xff0000) >> 8) | (((_x) & 0xff000000) >> 24))
#endif

#define SMBUS_IO_OFFSET	0x1100

#define R_SYSCON	ta2
#define R_MEM_TYPE	s0
#define R_NROWS		s4
#define R_NROW_ADDR	s5
#define R_NCOL_ADDR	s6
#define R_RETURN	s7

	.text

NESTED(lowlevel_init, 0, ra)
	/* save return register */
	move	R_RETURN, ra

	/* mux SMBus host controller to I2C bus */
	PTR_LI	t0, CKSEG1ADDR(0x1f000b18)
	sw	zero, 0(t0)

	/* detect the core card */
	PTR_LI	t0, CKSEG1ADDR(MALTA_REVISION)
	lw	t0, 0(t0)
	srl	t0, t0, MALTA_REVISION_CORID_SHF
	andi	R_SYSCON, t0, (MALTA_REVISION_CORID_MSK >> \
			 MALTA_REVISION_CORID_SHF)

	/* core cards using the gt64120 system controller */
	li	t1, MALTA_REVISION_CORID_CORE_LV
	beq	R_SYSCON, t1, _gt64120

	/* core cards using the MSC01 system controller */
	li	t1, MALTA_REVISION_CORID_CORE_FPGA6
	beq	R_SYSCON, t1, _msc01

	/* unknown system controller */
	b	.

	/*
	 * Load BAR registers of GT64120 as done by YAMON
	 *
	 * based on a patch sent by Antony Pavlov <antonynpavlov@gmail.com>
	 * to the barebox mailing list.
	 * The subject of the original patch:
	 *   'MIPS: qemu-malta: add YAMON-style GT64120 memory map'
	 * URL:
	 * http://www.mail-archive.com/barebox@lists.infradead.org/msg06128.html
	 *
	 * based on write_bootloader() in qemu.git/hw/mips_malta.c
	 * see GT64120 manual and qemu.git/hw/gt64xxx.c for details
	 */
_gt64120:
	/* move GT64120 registers from 0x14000000 to 0x1be00000 */
	PTR_LI	t1, CKSEG1ADDR(GT_DEF_BASE)
	li	t0, CPU_TO_GT32(0xdf000000)
	sw	t0, GT_ISD_OFS(t1)

	/* setup MEM-to-PCI0 mapping */
	PTR_LI	t1, CKSEG1ADDR(MALTA_GT_BASE)

	/* setup PCI0 io window to 0x18000000-0x181fffff */
	li	t0, CPU_TO_GT32(0xc0000000)
	sw	t0, GT_PCI0IOLD_OFS(t1)
	li	t0, CPU_TO_GT32(0x40000000)
	sw	t0, GT_PCI0IOHD_OFS(t1)

	/* setup PCI0 mem windows */
	li	t0, CPU_TO_GT32(0x80000000)
	sw	t0, GT_PCI0M0LD_OFS(t1)
	li	t0, CPU_TO_GT32(0x3f000000)
	sw	t0, GT_PCI0M0HD_OFS(t1)

	li	t0, CPU_TO_GT32(0xc1000000)
	sw	t0, GT_PCI0M1LD_OFS(t1)
	li	t0, CPU_TO_GT32(0x5e000000)
	sw	t0, GT_PCI0M1HD_OFS(t1)

#define SMBUS_DEVFN \
	((0xa << 1) | (0x3 << 8))

	/* disable SMBus host controller I/O accesses */
	PTR_LI	t0, CKSEG1ADDR(MALTA_GT_PCIIO_BASE)
	li	t2, (1 << 31) | SMBUS_DEVFN | PCI_COMMAND
	sw	t2, 0xcf8(t0)
	sw	zero, 0xcfc(t0)

	/* set SMBus host controller I/O base address */
	li	t2, (1 << 31) | SMBUS_DEVFN | 0x90
	sw	t2, 0xcf8(t0)
	li	t3, SMBUS_IO_OFFSET
	sw	t3, 0xcfc(t0)

	/* enable SMBus host controller */
	li	t2, (1 << 31) | SMBUS_DEVFN | 0xd0
	sw	t2, 0xcf8(t0)
	li	t3, 0x1 << 16
	sw	t3, 0xcfc(t0)

	/* enable SMBus host controller I/O accesses */
	li	t2, (1 << 31) | SMBUS_DEVFN | PCI_COMMAND
	sw	t2, 0xcf8(t0)
	li	t3, PCI_COMMAND_IO
	sw	t3, 0xcfc(t0)

#undef SMBUS_DEVFN

	jr	R_RETURN

	/*
	 *
	 */
_msc01:
	/* setup peripheral bus controller clock divide */
	PTR_LI	t0, CKSEG1ADDR(MALTA_MSC01_PBC_BASE)
	li	t1, 0x1 << MSC01_PBC_CLKCFG_SHF
	sw	t1, MSC01_PBC_CLKCFG_OFS(t0)

	/* tweak peripheral bus controller timings */
	li	t1, (0x1 << MSC01_PBC_CS0TIM_CDT_SHF) | \
		    (0x1 << MSC01_PBC_CS0TIM_CAT_SHF)
	sw	t1, MSC01_PBC_CS0TIM_OFS(t0)
	li	t1, (0x0 << MSC01_PBC_CS0RW_RDT_SHF) | \
		    (0x2 << MSC01_PBC_CS0RW_RAT_SHF) | \
		    (0x0 << MSC01_PBC_CS0RW_WDT_SHF) | \
		    (0x2 << MSC01_PBC_CS0RW_WAT_SHF)
	sw	t1, MSC01_PBC_CS0RW_OFS(t0)
	lw	t1, MSC01_PBC_CS0CFG_OFS(t0)
	li	t2, MSC01_PBC_CS0CFG_DTYP_MSK
	and	t1, t2
	ori	t1, (0x0 << MSC01_PBC_CS0CFG_ADM_SHF) | \
		    (0x3 << MSC01_PBC_CS0CFG_WSIDLE_SHF) | \
		    (0x10 << MSC01_PBC_CS0CFG_WS_SHF)
	sw	t1, MSC01_PBC_CS0CFG_OFS(t0)

	/* setup basic address decode */
	PTR_LI	t0, CKSEG1ADDR(MALTA_MSC01_BIU_BASE)
	li	t1, 0x0
	li	t2, -CONFIG_SYS_MEM_SIZE
	sw	t1, MSC01_BIU_MCBAS1L_OFS(t0)
	sw	t2, MSC01_BIU_MCMSK1L_OFS(t0)
	sw	t1, MSC01_BIU_MCBAS2L_OFS(t0)
	sw	t2, MSC01_BIU_MCMSK2L_OFS(t0)

	/* initialise IP1 - memory controller */
	li	t1, MALTA_MSC01_IP1_BASE
	li	t2, -MALTA_MSC01_IP1_SIZE
	sw	t1, MSC01_BIU_IP1BAS1L_OFS(t0)
	sw	t2, MSC01_BIU_IP1MSK1L_OFS(t0)
	sw	t1, MSC01_BIU_IP1BAS2L_OFS(t0)
	sw	t2, MSC01_BIU_IP1MSK2L_OFS(t0)

	/* initialise IP2 - PCI */
	li	t1, MALTA_MSC01_IP2_BASE1
	li	t2, -MALTA_MSC01_IP2_SIZE1
	sw	t1, MSC01_BIU_IP2BAS1L_OFS(t0)
	sw	t2, MSC01_BIU_IP2MSK1L_OFS(t0)
	li	t1, MALTA_MSC01_IP2_BASE2
	li	t2, -MALTA_MSC01_IP2_SIZE2
	sw	t1, MSC01_BIU_IP2BAS2L_OFS(t0)
	sw	t2, MSC01_BIU_IP2MSK2L_OFS(t0)

	/* initialise IP3 - peripheral bus controller */
	li	t1, MALTA_MSC01_IP3_BASE
	li	t2, -MALTA_MSC01_IP3_SIZE
	sw	t1, MSC01_BIU_IP3BAS1L_OFS(t0)
	sw	t2, MSC01_BIU_IP3MSK1L_OFS(t0)
	sw	t1, MSC01_BIU_IP3BAS2L_OFS(t0)
	sw	t2, MSC01_BIU_IP3MSK2L_OFS(t0)

	/* setup PCI memory */
	PTR_LI	t0, CKSEG1ADDR(MALTA_MSC01_PCI_BASE)
	li	t1, MALTA_MSC01_PCIMEM_BASE
	li	t2, (-MALTA_MSC01_PCIMEM_SIZE) & MSC01_PCI_SC2PMMSKL_MSK_MSK
	li	t3, MALTA_MSC01_PCIMEM_MAP
	sw	t1, MSC01_PCI_SC2PMBASL_OFS(t0)
	sw	t2, MSC01_PCI_SC2PMMSKL_OFS(t0)
	sw	t3, MSC01_PCI_SC2PMMAPL_OFS(t0)

	/* setup PCI I/O */
	li	t1, MALTA_MSC01_PCIIO_BASE
	li	t2, (-MALTA_MSC01_PCIIO_SIZE) & MSC01_PCI_SC2PIOMSKL_MSK_MSK
	li	t3, MALTA_MSC01_PCIIO_MAP
	sw	t1, MSC01_PCI_SC2PIOBASL_OFS(t0)
	sw	t2, MSC01_PCI_SC2PIOMSKL_OFS(t0)
	sw	t3, MSC01_PCI_SC2PIOMAPL_OFS(t0)

	/* setup PCI_BAR0 memory window */
	li	t1, -CONFIG_SYS_MEM_SIZE
	sw	t1, MSC01_PCI_BAR0_OFS(t0)

	/* setup PCI to SysCon/CPU translation */
	sw	t1, MSC01_PCI_P2SCMSKL_OFS(t0)
	sw	zero, MSC01_PCI_P2SCMAPL_OFS(t0)

	/* setup PCI vendor & device IDs */
	li	t1, (PCI_VENDOR_ID_MIPS << MSC01_PCI_HEAD0_VENDORID_SHF) | \
		    (PCI_DEVICE_ID_MIPS_MSC01 << MSC01_PCI_HEAD0_DEVICEID_SHF)
	sw	t1, MSC01_PCI_HEAD0_OFS(t0)

	/* setup PCI subsystem vendor & device IDs */
	sw	t1, MSC01_PCI_HEAD11_OFS(t0)

	/* setup PCI class, revision */
	li	t1, (PCI_CLASS_BRIDGE_HOST << MSC01_PCI_HEAD2_CLASS_SHF) | \
		    (0x1 << MSC01_PCI_HEAD2_REV_SHF)
	sw	t1, MSC01_PCI_HEAD2_OFS(t0)

	/* ensure a sane setup */
	sw	zero, MSC01_PCI_HEAD3_OFS(t0)
	sw	zero, MSC01_PCI_HEAD4_OFS(t0)
	sw	zero, MSC01_PCI_HEAD5_OFS(t0)
	sw	zero, MSC01_PCI_HEAD6_OFS(t0)
	sw	zero, MSC01_PCI_HEAD7_OFS(t0)
	sw	zero, MSC01_PCI_HEAD8_OFS(t0)
	sw	zero, MSC01_PCI_HEAD9_OFS(t0)
	sw	zero, MSC01_PCI_HEAD10_OFS(t0)
	sw	zero, MSC01_PCI_HEAD12_OFS(t0)
	sw	zero, MSC01_PCI_HEAD13_OFS(t0)
	sw	zero, MSC01_PCI_HEAD14_OFS(t0)
	sw	zero, MSC01_PCI_HEAD15_OFS(t0)

	/* setup PCI command register */
	li	t1, (PCI_COMMAND_FAST_BACK | \
		     PCI_COMMAND_SERR | \
		     PCI_COMMAND_PARITY | \
		     PCI_COMMAND_MASTER | \
		     PCI_COMMAND_MEMORY)
	sw	t1, MSC01_PCI_HEAD1_OFS(t0)

	/* setup PCI byte swapping */
#ifdef CONFIG_SYS_BIG_ENDIAN
	li	t1, (0x1 << MSC01_PCI_SWAP_BAR0_BSWAP_SHF) | \
		    (0x1 << MSC01_PCI_SWAP_IO_BSWAP_SHF)
	sw	t1, MSC01_PCI_SWAP_OFS(t0)
#else
	sw	zero, MSC01_PCI_SWAP_OFS(t0)
#endif

	/* enable PCI host configuration cycles */
	lw	t1, MSC01_PCI_CFG_OFS(t0)
	li	t2, MSC01_PCI_CFG_RA_MSK | \
		    MSC01_PCI_CFG_G_MSK | \
		    MSC01_PCI_CFG_EN_MSK
	or	t1, t1, t2
	sw	t1, MSC01_PCI_CFG_OFS(t0)

#define SMBUS_DEVFN \
	((0xa << MSC01_PCI_CFGADDR_DNUM_SHF) | \
	 (0x3 << MSC01_PCI_CFGADDR_FNUM_SHF))

	/* disable SMBus host controller I/O accesses */
	li	t1, MSC01_PCI_INTSTAT_MA_MSK | MSC01_PCI_INTSTAT_TA_MSK
	sw	t1, MSC01_PCI_INTSTAT_OFS(t0)
	li	t2, SMBUS_DEVFN | PCI_COMMAND
	sw	t2, MSC01_PCI_CFGADDR_OFS(t0)
	sw	zero, MSC01_PCI_CFGDATA_OFS(t0)

	/* set SMBus host controller I/O base address */
	sw	t1, MSC01_PCI_INTSTAT_OFS(t0)
	li	t2, SMBUS_DEVFN | 0x90
	sw	t2, MSC01_PCI_CFGADDR_OFS(t0)
	li	t3, SMBUS_IO_OFFSET
	sw	t3, MSC01_PCI_CFGDATA_OFS(t0)

	/* enable SMBus host controller */
	sw	t1, MSC01_PCI_INTSTAT_OFS(t0)
	li	t2, SMBUS_DEVFN | 0xd0
	sw	t2, MSC01_PCI_CFGADDR_OFS(t0)
	li	t3, 0x1 << 16
	sw	t3, MSC01_PCI_CFGDATA_OFS(t0)

	/* enable SMBus host controller I/O accesses */
	sw	t1, MSC01_PCI_INTSTAT_OFS(t0)
	li	t2, SMBUS_DEVFN | PCI_COMMAND
	sw	t2, MSC01_PCI_CFGADDR_OFS(t0)
	li	t3, PCI_COMMAND_IO
	sw	t3, MSC01_PCI_CFGDATA_OFS(t0)

#undef SMBUS_DEVFN

	/* detect the type of memory */
	li	a0, SPD_MEM_TYPE
	bal	malta_spd_read
	move	R_MEM_TYPE, v0

	/* check if we have DDR or DDR2 */
	li	t0, SPD_MEMTYPE_DDR
	beq	R_MEM_TYPE, t0, 2f
	li	t0, SPD_MEMTYPE_DDR2
	beq	R_MEM_TYPE, t0, 2f

	/* check if we have non-DDR SDRAM */
	li	t0, SPD_MEMTYPE_SDRAM
	beq	R_MEM_TYPE, t0, 2f

	/* bad memory type - hang */
	PTR_LA	a0, msg_mem_type
	bal	malta_lcd_puts
1:	wait
	b	1b

	/* read module banks from SPD */
2:	li	a0, SPD_NROWS
	bal	malta_spd_read
	move	R_NROWS, v0

	/* handle DDR2 bank encoding */
	li	t1, SPD_MEMTYPE_DDR2
	bltu	R_MEM_TYPE, t1, 1f
	andi	R_NROWS, R_NROWS, 0x7
	addiu	R_NROWS, R_NROWS, 1

	/* read number of banks */
1:	li	a0, SPD_NBANKS
	bal	malta_spd_read
	move	t1, v0

	/* read number of row addresses */
	li	a0, SPD_NROW_ADDR
	bal	malta_spd_read
	andi	R_NROW_ADDR, v0, 0xf

	/* read number of column addresses */
	li	a0, SPD_NCOL_ADDR
	bal	malta_spd_read
	andi	R_NCOL_ADDR, v0, 0xf

	/* use less column address bits than row address bits */
	addiu	t1, R_NROW_ADDR, -1
	ble	R_NCOL_ADDR, t1, 1f
	move	R_NCOL_ADDR, t1

	/* sum of rows & columns must not exceed 25 */
1:	li	t1, 25
	sub	t1, t1, R_NROW_ADDR
	blt	R_NCOL_ADDR, t1, 1f
	move	R_NCOL_ADDR, t1

	/* skip over CoreFPGA6 path if not running on CoreFPGA6 */
1:	li	ta0, MALTA_REVISION_CORID_CORE_FPGA6
	bne	R_SYSCON, ta0, 1f

	/* auto-tune CoreFPGA6 memory controller timings */
	log2	ta1, R_NROWS
	sll	v0, ta1, MALTA_MSC01_CF6_MC_TUNE_NROWS_SHF
	log2	ta1, t1
	sll	ta1, ta1, MALTA_MSC01_CF6_MC_TUNE_NBANKS_SHF
	or	v0, v0, ta1
	sll	ta0, R_NROW_ADDR, MALTA_MSC01_CF6_MC_TUNE_NROW_ADDR_SHF
	or	v0, v0, ta0
	or	v0, v0, R_NCOL_ADDR
	PTR_LI	ta0, CKSEG1ADDR(MALTA_MSC01_CF6_MC_TUNE)
	sw	v0, 0(ta0)

1:	jr	R_RETURN
	END(lowlevel_init)

LEAF(malta_lcd_puts)
	.set	push
	.set	noat
	PTR_LI	v0, CKSEG1ADDR(MALTA_ASCIIPOS0)
	PTR_ADDIU \
		v1, v0, 8 * (MALTA_ASCIIPOS1 - MALTA_ASCIIPOS0)
1:	lbu	$1, 0(a0)
	beqz	$1, 3f
	PTR_ADDIU \
		a0, a0, 1
2:	sw	$1, 0(v0)
	PTR_ADDIU \
		v0, v0, MALTA_ASCIIPOS1 - MALTA_ASCIIPOS0
	bne	v0, v1, 1b

	/* return */
	jr	ra

	/* out of line short-string handling */
3:	li	$1, ' '
	b	2b
	.set	pop
	END(malta_lcd_puts)

LEAF(malta_spd_read)
	.set	push
	.set	noat
	PTR_LI	$1, CKSEG1ADDR(MALTA_REVISION)
	lw	$1, 0($1)
	srl	$1, $1, MALTA_REVISION_CORID_SHF
	and	$1, $1, MALTA_REVISION_CORID_MSK >> MALTA_REVISION_CORID_SHF
	li	v0, MALTA_REVISION_CORID_CORE_LV
	PTR_LI	v1, CKSEG1ADDR(MALTA_MSC01_PCIIO_BASE + SMBUS_IO_OFFSET)
	bne	$1, v0, 1f
	PTR_LI	v1, CKSEG1ADDR(MALTA_GT_PCIIO_BASE + SMBUS_IO_OFFSET)

1:	li	$1, (0x50 << SMBXMITADD_ADDR_SHF) | SMBXMITADD_READ
	sb	$1, SMBXMITADD(v1)

	sb	a0, SMBHSTCMD(v1)

	/* wait until ready */
1:	lbu	$1, SMBHSTSTAT(v1)
	sb	$1, SMBHSTSTAT(v1)
	bnez	$1, 1b

	/* start transaction */
	li	$1, SMBHSTCNT_START | SMBHSTCNT_CMD_BYTE_DATA
	sb	$1, SMBHSTCTL(v1)

	/* wait until done */
1:	lbu	$1, SMBHSTSTAT(v1)
	andi	$1, $1, SMBHSTSTS_INTR | SMBHSTSTS_HOST_BUSY
	xori	$1, $1, SMBHSTSTS_INTR
	bnez	$1, 1b

	/* load the data */
	lbu	v0, SMBHSTDAT0(v1)

	/* return */
	jr	ra
	.set	pop
	END(malta_spd_read)

.data

msg_mem_type:	.asciiz "Mem Type"
