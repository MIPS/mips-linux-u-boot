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

#define MIN_PERIOD	8
#define MAX_PERIOD	60

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
	li	t1, MALTA_REVISION_CORID_CORE_FPGA4
	beq	R_SYSCON, t1, _msc01
	li	t1, MALTA_REVISION_CORID_CORE_FPGA5
	beq	R_SYSCON, t1, _msc01
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
	PTR_LA	t2, denali_ddr_regs
	li	t0, SPD_MEMTYPE_DDR
	beq	R_MEM_TYPE, t0, 2f
	li	t0, SPD_MEMTYPE_DDR2
	beq	R_MEM_TYPE, t0, 2f

	/* check if we have non-DDR SDRAM */
	PTR_LA	t2, denali_sdr_regs
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

	/* that's all we need to do for CoreFPGA6 :) */
	jr	R_RETURN

	/* pre-CoreFPGA6, write base register settings */
1:	PTR_LI	t0, CKSEG1ADDR(MALTA_MSC01_IP1_BASE)
	move	t1, t0
	PTR_ADDIU \
		t3, t2, denali_ddr_regs_end - denali_ddr_regs
1:	lw	ta0, 0(t2)
	addiu	t2, t2, 4
	sw	ta0, 0(t1)
	addiu	t1, t1, 4
	bne	t2, t3, 1b

	/* check for DDR_CTRL presence */
	PTR_LI	t1, CKSEG1ADDR(MALTA_DDR_CTRL)
	lw	t2, 0(t1)
	and	t3, t2, MALTA_DDR_CTRL_PRESENT
	beqz	t3, 3f

	/* set burst length to 2 */
	lw	t3, (27 * 4)(t0)
	ori	t3, t3, 0x7
	xori	t3, t3, 0x6
	sw	t3, (27 * 4)(t0)

	/* check ECC only bit */
	and	t3, t2, MALTA_DDR_CTRL_ECCONLY
	bnez	t3, 1f

	/* no RMW for CPU write bursts */
	or	t2, t2, MALTA_DDR_CTRL_WRBURST_FULL

	/* check if this DDR has ECC */
	bltu	R_MEM_TYPE, SPD_MEMTYPE_DDR, 2f
	li	a0, SPD_CONFIG
	bal	malta_spd_read
	li	t3, SPD_CONFIG_ECC
	bne	v0, t3, 2f

	/* reuse data mask pins as data strobes with ECC DDR */
1:	or	t2, t2, MALTA_DDR_CTRL_DM2DQS

	/* enable RMW */
2:	or	t2, t2, MALTA_DDR_CTRL_RMW

	/* update DDR_CTRL register, load acts as a barrier */
	sw	t2, 0(t1)
	lw	t2, 0(t1)

	/* mask all interrupts */
3:	lw	t1, (10 * 4)(t0)
	or	t1, t1, 0x1f << 16
	sw	t1, (10 * 4)(t0)

	/* set registered DIMM bit */
	li	a0, SPD_MOD_ATTR
	bal	malta_spd_read
	srl	v0, v0, SPD_MOD_ATTR_REG_SHF
	andi	v0, v0, 1
	lw	t1, (2 * 4)(t0)
	ins	t1, v0, 24, 1
	sw	t1, (2 * 4)(t0)

	/* find max row & col address bits */
	lw	t1, (9 * 4)(t0)
	andi	t2, t1, 0xf
	srl	t1, t1, 8
	andi	t1, t1, 0xf

	/* set addr_pins = addrbits_max - nrow_addr */
	sub	t1, t1, R_NROW_ADDR
	lw	t3, (5 * 4)(t0)
	ins	t3, t1, 24, 3
	sw	t3, (5 * 4)(t0)

	/* set column_size = colbits_max - ncol_addr */
	sub	t2, t2, R_NCOL_ADDR
	lw	t3, (6 * 4)(t0)
	ins	t3, t2, 8, 3
	sw	t3, (6 * 4)(t0)

	/* Set csmap = (1 << nrows) - 1 */
	li	t1, 1
	sllv	t1, t1, R_NROWS
	addiu	t1, t1, -1
	lw	t2, (4 * 4)(t0)
	ins	t2, t1, 24, 2
	sw	t2, (4 * 4)(t0)

	/* read CAS latency */
	li	a0, SPD_CAS_LAT
	bal	malta_spd_read

	/* skip DDR path for non-DDR SDRAM */
	bltu	R_MEM_TYPE, SPD_MEMTYPE_DDR, 3f

#define HANDLE_CAS(spd_lat, mc_lat, mc_oth)		\
1:	and	t1, v0, SPD_CAS_LAT_DDR_##spd_lat;	\
	beqz	t1, 1f;					\
	li	t2, mc_lat;				\
	li	t3, mc_oth;				\
	b	2f

	HANDLE_CAS(1_0, 1, 2)
	HANDLE_CAS(1_5, 5, 3)
	HANDLE_CAS(2_0, 2, 4)
	HANDLE_CAS(2_5, 6, 5)
	HANDLE_CAS(3_0, 3, 6)

#undef HANDLE_CAS

	/* no valid CAS latency */
1:	PTR_LA	a0, msg_bad_cas
	bal	malta_lcd_puts
1:	wait
	b	1b

	/* set CAS latency */
2:	lw	t1, (6 * 4)(t0)
	ins	t1, t2, 0, 3
	sw	t1, (6 * 4)(t0)

	/* set CAS things */
	lw	t1, (10 * 4)(t0)
	ins	t1, t3, 0, 4
	ins	t1, t3, 8, 4
	sw	t1, (10 * 4)(t0)

	/* skip non-DDR path for DDR */
	b	5f

	/* select CAS latency 2 or 3, preferring 2 */
3:	and	t1, v0, SPD_CAS_LAT_2
	li	t2, 2
	bnez	t1, 4f
	li	t2, 3

	/* set CAS latency */
4:	lw	t1, (6 * 4)(t0)
	ins	t1, t2, 0, 3
	sw	t1, (6 * 4)(t0)

	/* set CAS things */
	lw	t1, (10 * 4)(t0)
	sll	t2, t2, 1
	ins	t1, t2, 0, 4
	ins	t1, t2, 8, 4
	sw	t1, (10 * 4)(t0)

	/* read tRP */
	li	a0, SPD_TRP
	bal	malta_spd_read
	li	t1, MIN_PERIOD
	divu	v0, t1
#if __mips_isa_rev < 6
	mflo	v0
#endif

	/* clamp tRP >= 2 */
	bgeu	v0, 2, 1f
	li	v0, 2

1:	/* set tRP */
	lw	t1, (9 * 4)(t0)
	move	t3, v0
	ins	t1, v0, 24, 4
	sw	t1, (9 * 4)(t0)

	/* read tRCD */
	li	a0, SPD_TRCD
	bal	malta_spd_read
	li	t1, MIN_PERIOD
	divu	v0, t1
#if __mips_isa_rev < 6
	mflo	v0
#endif

	/* clamp tRCD >= 2 */
	bgeu	v0, 2, 1f
	li	v0, 2

1:	/* set tRCD */
	lw	t1, (17 * 4)(t0)
	ins	t1, v0, 24, 8
	sw	t1, (17 * 4)(t0)

	/* read tRRD */
	li	a0, SPD_TRRD
	bal	malta_spd_read
	li	t1, MIN_PERIOD
	divu	v0, t1
#if __mips_isa_rev < 6
	mflo	v0
#endif

	/* set tRRD */
	lw	t1, (7 * 4)(t0)
	ins	t1, v0, 16, 3
	sw	t1, (7 * 4)(t0)

	/* read tRAS */
	li	a0, SPD_TRAS
	bal	malta_spd_read
	li	t1, MIN_PERIOD
	divu	v0, t1
#if __mips_isa_rev < 6
	mflo	v0
#endif

	/* clamp tRAS >= 6 */
	bgeu	v0, 6, 1f
	li	v0, 6

1:	/* set tRAS_min */
	lw	t1, (17 * 4)(t0)
	addu	t3, t3, v0
	ins	t1, v0, 16, 8
	sw	t1, (17 * 4)(t0)

	/* read tRC */
	li	a0, SPD_TRC
	bal	malta_spd_read
	li	t1, MIN_PERIOD
	divu	v0, t1
#if __mips_isa_rev < 6
	mflo	v0
#endif

	/* clamp tRC >= tRP + tRAS_min */
	bgeu	v0, t3, 1f
	move	v0, t3

1:	/* set tRC */
	lw	t1, (11 * 4)(t0)
	ins	t1, v0, 8, 5
	sw	t1, (11 * 4)(t0)

	/* set tRAS_max to 100000ns */
	li	v0, (100000 + MAX_PERIOD - 1) / MAX_PERIOD
	lw	t1, (20 * 4)(t0)
	ins	t1, v0, 0, 16
	sw	t1, (20 * 4)(t0)

	/* set tWR to 15ns */
	li	v0, (15 + MIN_PERIOD - 1) / MIN_PERIOD
	lw	t1, (7 * 4)(t0)
	ins	t1, v0, 24, 3
	sw	t1, (7 * 4)(t0)

	/* read refresh rate */
5:	li	a0, SPD_REFRESH
	bal	malta_spd_read
	and	v0, v0, SPD_REFRESH_RATE

#define HANDLE_RATE(spd_rate, mc_rate)			\
1:	li	t1, SPD_REFRESH_RATE_##spd_rate;	\
	bne	v0, t1, 1f;				\
	li	t2, (mc_rate & ~1) / MAX_PERIOD;	\
	b	2f

	HANDLE_RATE(125, 125000)
	HANDLE_RATE(62_5, 62500)
	HANDLE_RATE(31_3, 31300)
	HANDLE_RATE(15_625, 15625)
	HANDLE_RATE(7_8, 7800)

#undef HANDLE_RATE

	/* default to 3.9uS */
1:	li	t2, 3900 / MAX_PERIOD

	/* set Tref */
2:	lw	t1, (18 * 4)(t0)
	ins	t1, t2, 8, 12
	sw	t1, (18 * 4)(t0)

	/* set Tref enable */
	lw	t1, (4 * 4)(t0)
	or	t1, t1, 0x1
	sw	t1, (4 * 4)(t0)

	/* update the user */
	PTR_LA	a0, msg_mc_en
	bal	malta_lcd_puts

	/* enable the memory controller */
	lw	t1, 0xc(t0)
	or	t1, t1, 1 << 16
	sw	t1, 0xc(t0)

	/* wait for init complete interrupt */
1:	lw	t1, 0x28(t0)
	srl	t1, t1, 24
	andi	t1, t1, 0x1f
	beqz	t1, 1b

	/* acknowledge the interrupt */
	lw	t1, (8 * 4)(t0)
	or	t1, t1, 0xf << 24
	sw	t1, (8 * 4)(t0)

	/* unmask interrupts */
	lw	t1, (10 * 4)(t0)
	xor	t1, t1, 0x1f << 16
	sw	t1, (10 * 4)(t0)

	/* disable DDR's DLL - we're running too slowly for it */
	lw	t1, (28 * 4)(t0)
	li	t2, 1
	ins	t1, t2, 16, 14
	sw	t1, (28 * 4)(t0)

	lw	t1, (4 * 4)(t0)
	or	t1, t1, 1 << 16
	sw	t1, (4 * 4)(t0)

	/* delay */
	li	t1, 0xffff
1:	addiu	t1, -1
	bnez	t1, 1b

	/* select a stride - 16 bytes if RMW, otherwise 4 bytes */
	li	ta0, 4
	PTR_LI	t1, CKSEG1ADDR(MALTA_DDR_CTRL)
	and	t1, t1, MALTA_DDR_CTRL_RMW
	beqz	t1, 1f
	li	ta0, 16

	/* setup for pattern loops */
1:	PTR_LI	t1, CKSEG1ADDR(0)
	li	t2, 0xaa55aa00
	li	t3, 0xaa55aa15
	li	v0, 2
	li	v1, 11

	/* write a pattern at the start of DDR */
	move	ta1, t1
	move	ta2, t2
1:	sw	ta2, 0(ta1)
	addiu	ta2, ta2, 1
	addu	ta1, ta1, ta0
	bleu	ta2, t3, 1b

	/* set the CAS latency */
1:	lw	ta1, (10 * 4)(t0)
	ins	ta1, v0, 0, 4
	ins	ta1, v0, 8, 4
	sw	ta1, (10 * 4)(t0)

	/* check the pattern */
	move	ta1, t1
	move	ta2, t2
2:	lw	ta3, 0(ta1)
	bne	ta3, ta2, 3f
	addiu	ta2, ta2, 1
	addu	ta1, ta1, ta0
	bleu	ta2, t3, 2b

	/* pattern read back OK */
	PTR_LA	a0, msg_mc_ok
	bal	malta_lcd_puts

	/* return */
	jr	R_RETURN

3:	/* pattern read incorrectly, next latency */
	addiu	v0, v0, 1
	bltu	v0, v1, 1b

	/* pattern read failed for all latencies */
	PTR_LA	a0, msg_bad_ram
	bal	malta_lcd_puts
1:	wait
	b	1b
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

denali_sdr_regs:
	.word 0x00000000, 0x00000100, 0x00000000, 0x00000001
	.word 0x03000101, 0x03020000, 0x00000502, 0x02020000
	.word 0x00080a01, 0x02040000, 0x00000404, 0x00090602
	.word 0x0a0a001e, 0x00000a0a, 0x04040000, 0x00000404
	.word 0x01010000, 0x02040101, 0x000c2d06, 0x00000000
	.word 0x00062706, 0x00000006, 0x00000014, 0x00000000
	.word 0x01010101, 0x01010000, 0x00010100, 0x0f0f0002
	.word 0x00000000, 0x00000000, 0x00000000, 0x00010000
denali_sdr_regs_end:

denali_ddr_regs:
	.word 0x01000000, 0x01000100, 0x00000001, 0x01000000
	.word 0x03000101, 0x02020000, 0x00000403, 0x02010000
	.word 0x00020a01, 0x02040000, 0x00000606, 0x00090602
	.word 0x2020001e, 0x00002020, 0x0b0b0000, 0x00000b0b
	.word 0x01010000, 0x02040101, 0x00030507, 0x00c80000
	.word 0x00c80618, 0x000000c8, 0x00000014, 0x00000000
	.word 0x01010101, 0x01010000, 0x00010100, 0x0f0f0002
	.word 0x00000000, 0x00000000, 0x00000000, 0x00000000
denali_ddr_regs_end:

msg_mem_type:	.asciiz "Mem Type"
msg_bad_cas:	.asciiz "Bad CAS"
msg_bad_ram:	.asciiz "Bad RAM"
msg_mc_en:	.asciiz "MC Start"
msg_mc_ok:	.asciiz "MC OK"
