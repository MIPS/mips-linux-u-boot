/* SPDX-License-Identifier: GPL-2.0+ */
/*
 * MIPS Coherence Manager (CM) Register Definitions
 *
 * Copyright (c) 2016 Imagination Technologies Ltd.
 */
#ifndef __MIPS_ASM_CM_H__
#define __MIPS_ASM_CM_H__

/* Global Control Register (GCR) offsets */
#define GCR_CONFIG			0x0000
#define GCR_BASE			0x0008
#define GCR_BASE_UPPER			0x000c
#define GCR_REV				0x0030
#define GCR_CPC_BASE			0x0088
#define GCR_CPC_STATUS			0x00f0
#define GCR_L2_CONFIG			0x0130
#define GCR_SYS_CONFIG			0x0140
#define GCR_L2_TAG_ADDR			0x0600
#define GCR_L2_TAG_ADDR_UPPER		0x0604
#define GCR_L2_TAG_STATE		0x0608
#define GCR_L2_TAG_STATE_UPPER		0x060c
#define GCR_L2_DATA			0x0610
#define GCR_L2_DATA_UPPER		0x0614
#define GCR_L2_ECC			0x0618
#define GCR_L2SM_COP			0x0620
#define  GCR_L2SM_COP_PRESENT		(0x1 << 31)
#define  GCR_L2SM_COP_RESULT		(0x7 << 6)
#define  GCR_L2SM_COP_RESULT_DONE_OK	(0x1 << 6)
#define  GCR_L2SM_COP_RUNNING		(0x1 << 5)
#define  GCR_L2SM_COP_TYPE_STORE_TAG	(0x1 << 2)
#define  GCR_L2SM_COP_CMD_START		(0x1 << 0)
#define GCR_MMIO_REQ_LIMIT		0x06f8
#define GCR_MMIO0_BOTTOM		0x0700
#define  GCR_MMIO0_BOTTOM_ADDR		(0xffffffffull << 16)
#define  GCR_MMIO0_BOTTOM_PORT_SHIFT	2
#define  GCR_MMIO0_BOTTOM_PORT		(0xf << 2)
#define  GCR_MMIO0_BOTTOM_DISABLE_LIMIT	(0x1 << 1)
#define  GCR_MMIO0_BOTTOM_ENABLE	(0x1 << 0)
#define GCR_MMIO0_TOP			0x0708
#define  GCR_MMIO0_TOP_ADDR		(0xffffffffull << 16)
#define GCR_MMIO1_BOTTOM		0x0710
#define GCR_Cx_COHERENCE		0x2008
#define GCR_Cx_REDIRECT			0x2018
#define GCR_Cx_ID			0x2028

/* Block offsets */
#define GCR_OFF_GLOBAL			0x0000
#define GCR_OFF_LOCAL			0x2000
#define GCR_OFF_REDIRECT		0x4000

/* GCR_CONFIG */
#define GCR_CONFIG_NUMCLUSTERS_SHIFT	23
#define GCR_CONFIG_NUMCLUSTERS_BITS	(0x3f << 23)
#define GCR_CONFIG_NUMIOCU_SHIFT	8
#define GCR_CONFIG_NUMIOCU_BITS		(0xf << 8)
#define GCR_CONFIG_NUMCORES_SHIFT	0
#define GCR_CONFIG_NUMCORES_BITS	(0xff << 0)

/* GCR_REV CM versions */
#define GCR_REV_CM3			0x0800
#define GCR_REV_CM3_5			0x0900

/* GCR_L2_CONFIG fields */
#define GCR_L2_CONFIG_ASSOC_SHIFT	0
#define GCR_L2_CONFIG_ASSOC_BITS	8
#define GCR_L2_CONFIG_LINESZ_SHIFT	8
#define GCR_L2_CONFIG_LINESZ_BITS	4
#define GCR_L2_CONFIG_SETSZ_SHIFT	12
#define GCR_L2_CONFIG_SETSZ_BITS	4
#define GCR_L2_CONFIG_BYPASS		(1 << 20)

/* GCR_Cx_COHERENCE */
#define GCR_Cx_COHERENCE_DOM_EN		(0xff << 0)
#define GCR_Cx_COHERENCE_EN		(0x1 << 0)

/* GCR_Cx_REDIRECT */
#define GCR_Cx_REDIRECT_CLUSTER_EN	(0x1 << 31)
#define GCR_Cx_REDIRECT_GIC_EN		(0x1 << 30)
#define GCR_Cx_REDIRECT_BLOCK_SHIFT	24
#define GCR_Cx_REDIRECT_BLOCK_BITS	(0x3 << 24)
#define GCR_Cx_REDIRECT_BLOCK_GCR_CORE_LOCAL	0x0
#define GCR_Cx_REDIRECT_BLOCK_GCR_GLOBAL	0x1
#define GCR_Cx_REDIRECT_BLOCK_GCR_DEBUG		0x2
#define GCR_Cx_REDIRECT_BLOCK_CPC_CORE_LOCAL	0x0
#define GCR_Cx_REDIRECT_BLOCK_CPC_GLOBAL	0x1
#define GCR_Cx_REDIRECT_BLOCK_GIC_VP_LOCAL	0x0
#define GCR_Cx_REDIRECT_BLOCK_GIC_SHARED_LOWER	0x1
#define GCR_Cx_REDIRECT_BLOCK_GIC_USER		0x2
#define GCR_Cx_REDIRECT_BLOCK_GIC_SHARED_UPPER	0x3
#define GCR_Cx_REDIRECT_CLUSTER_SHIFT	16
#define GCR_Cx_REDIRECT_CLUSTER_BITS	(0x3f << 16)
#define GCR_Cx_REDIRECT_CORE_SHIFT	8
#define GCR_Cx_REDIRECT_CORE_BITS	(0x3f << 8)
#define GCR_Cx_REDIRECT_CORE_CM_MASTER	0x20
#define GCR_Cx_REDIRECT_VP_SHIFT	0
#define GCR_Cx_REDIRECT_VP_BITS		(0x7 << 0)

#define GCR_Cx_ID_CLUSTER_SHIFT		8
#define GCR_Cx_ID_CLUSTER_BITS		(0xff << 8)
#define GCR_Cx_ID_CORE_SHIFT		0
#define GCR_Cx_ID_CORE_BITS		(0xff << 0)

/* CPC Block offsets */
#define CPC_OFF_GLOBAL			0x0000
#define CPC_OFF_LOCAL			0x2000
#define CPC_OFF_REDIRECT		0x4000

#define CPC_PWRUP_CTL			0x0030
#define  CPC_PWRUP_CTL_PWRUP		(0x1 << 0)

#define CPC_CONFIG			0x138
#define CPC_SYS_CONFIG			0x140

#define CPC_Cx_CMD			0x0000
#define  CPC_Cx_CMD_RESET		0x4

#define CPC_Cx_STAT_CONF		0x0008
#define  CPC_Cx_STAT_CONF_SEQ_STATE	(0xf << 19)
#define  CPC_Cx_STAT_CONF_SEQ_STATE_U5	(0x6 << 19)

#ifndef __ASSEMBLY__

#include <asm/io.h>

static inline void *mips_cm_base(void)
{
	return (void *)CKSEG1ADDR(CONFIG_MIPS_CM_BASE);
}

static inline void *mips_cpc_base(void)
{
	return (void *)CKSEG1ADDR(CONFIG_MIPS_CPC_BASE);
}

static inline unsigned long mips_cm_l2_line_size(void)
{
	unsigned long l2conf, line_sz;

	l2conf = __raw_readl(mips_cm_base() + GCR_L2_CONFIG);

	line_sz = l2conf >> GCR_L2_CONFIG_LINESZ_SHIFT;
	line_sz &= GENMASK(GCR_L2_CONFIG_LINESZ_BITS - 1, 0);
	return line_sz ? (2 << line_sz) : 0;
}

static inline unsigned int mips_cm_num_clusters(void)
{
	u32 cfg;

	/*
	 * Multiple clusters are introduced with CM3.5, for earlier systems
	 * report a single cluster.
	 */
	if (__raw_readl(mips_cm_base() + GCR_REV) < GCR_REV_CM3_5)
		return 1;

	cfg = __raw_readl(mips_cm_base() + GCR_CONFIG);
	cfg &= GCR_CONFIG_NUMCLUSTERS_BITS;
	cfg >>= GCR_CONFIG_NUMCLUSTERS_SHIFT;

	return cfg;
}

struct mmio_region {
	phys_addr_t addr_low;
	phys_addr_t addr_high;
	unsigned int port : 4;
	unsigned int enable : 1;
};

extern const struct mmio_region *get_mmio_regions(void);

extern void setup_redirect(unsigned int cluster, unsigned int core,
			   unsigned int vp, unsigned int block);

extern int mips_cm_init(void);
extern int mips_cm_init_iocus(void);

extern int power_up_cluster(unsigned int cluster);
extern int power_down_cluster(unsigned int cluster);
extern int init_cluster_l2(unsigned int cluster);

#endif /* !__ASSEMBLY__ */

#endif /* __MIPS_ASM_CM_H__ */
