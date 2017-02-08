/*
 * MIPS Coherence Manager (CM) Support
 *
 * Copyright (c) 2016 Imagination Technologies Ltd.
 *
 * SPDX-License-Identifier:	GPL-2.0+
 */

#include <common.h>
#include <asm/cm.h>

void setup_redirect(unsigned int cluster, unsigned int core,
		    unsigned int vp, unsigned int block)
{
	u32 redir = 0;

	if (__raw_readl(mips_cm_base() + GCR_REV) >= GCR_REV_CM3_5) {
		redir |= GCR_Cx_REDIRECT_CLUSTER_EN;
		redir |= GCR_Cx_REDIRECT_GIC_EN;
		redir |= cluster << GCR_Cx_REDIRECT_CLUSTER_SHIFT;
		redir |= block << GCR_Cx_REDIRECT_BLOCK_SHIFT;
	} else {
		/* CM < 3.5 doesn't support cluster or block redirects */
		assert(cluster == 0);
		assert(block == 0);
	}

	redir |= core << GCR_Cx_REDIRECT_CORE_SHIFT;
	redir |= vp << GCR_Cx_REDIRECT_VP_SHIFT;

	/*
	 * Set the redirect register & sync to ensure later memory accesses to
	 * GCRs cannot be reordered and handled before the new redirect value
	 * takes effect.
	 */
	__raw_writel(redir, mips_cm_base() + GCR_Cx_REDIRECT);
	sync();
}

static void mips_cpc_init(void)
{
	u32 status;

	status = __raw_readl(mips_cm_base() + GCR_CPC_STATUS);
	if (!(status & 0x1))
		return;

	__raw_writel(CONFIG_MIPS_CPC_BASE | 0x1, mips_cm_base() + GCR_CPC_BASE);
	sync();
}

__weak const struct mmio_region *get_mmio_regions(void)
{
	return NULL;
}

static void setup_mmio_limits(void)
{
	DECLARE_GLOBAL_DATA_PTR;
	void __iomem *gcrs = mips_cm_base();
	const struct mmio_region *rgn = get_mmio_regions();
	unsigned int num_clusters = mips_cm_num_clusters();
	unsigned int limit = CONFIG_MIPS_CM_MMIO_LIMIT / num_clusters;
	unsigned int cm_rev = __raw_readl(gcrs + GCR_REV);
	unsigned int cl, i, reg_off, mask, tgt;

	if (!rgn)
		return;

	if (cm_rev < GCR_REV_CM3) {
		reg_off = 0x90; /* GCR_REG0_BASE */

		for (i = 0; rgn[i].addr_high; i++) {
			if (!rgn[i].enable)
				tgt = 0x0; /* disabled */
			else if (gd->arch.num_iocus_usable > 0)
				tgt = 0x2; /* IOCU 0 */
			else
				tgt = 0x1; /* memory */

			mask = (rgn[i].addr_high | 0xffff) + 1;
			mask = GENMASK(31, ffs(mask) - 1);

			__raw_writel(rgn[i].addr_low, gcrs + reg_off);
			__raw_writel(mask | tgt, gcrs + reg_off + 0x8);

			reg_off += 0x10;
		}

		return;
	}

	if (cm_rev >= GCR_REV_CM3_5)
		gcrs += GCR_OFF_REDIRECT;

	for (cl = 0; cl < num_clusters; cl++) {
		if (cm_rev >= GCR_REV_CM3_5) {
			if (cl > 0) {
				setup_redirect(cl, 0, 0, GCR_Cx_REDIRECT_BLOCK_CPC_GLOBAL);

				/* Skip clusters we didn't power up */
				if (!__raw_readl(mips_cpc_base() + CPC_OFF_REDIRECT + CPC_PWRUP_CTL))
					continue;
			}

			setup_redirect(cl, 0, 0, GCR_Cx_REDIRECT_BLOCK_GCR_GLOBAL);
		}

		reg_off = GCR_MMIO0_BOTTOM;

		for (i = 0; rgn[i].addr_high; i++) {
			__raw_writeq(rgn[i].addr_high & GCR_MMIO0_TOP_ADDR,
				     gcrs + reg_off + (GCR_MMIO0_TOP - GCR_MMIO0_BOTTOM));

			__raw_writeq((rgn[i].addr_low & GCR_MMIO0_BOTTOM_ADDR) |
				     (rgn[i].port << GCR_MMIO0_BOTTOM_PORT_SHIFT) |
				     (rgn[i].enable ? GCR_MMIO0_BOTTOM_ENABLE : 0),
				     gcrs + reg_off);

			reg_off += GCR_MMIO1_BOTTOM - GCR_MMIO0_BOTTOM;
		}

		__raw_writel(limit, gcrs + GCR_MMIO_REQ_LIMIT);
	}
}

int power_up_cluster(unsigned int cluster)
{
	void __iomem *cpc_redir;
	unsigned int attempts;
	u32 stat;

	setup_redirect(cluster, GCR_Cx_REDIRECT_CORE_CM_MASTER, 0,
		       GCR_Cx_REDIRECT_BLOCK_CPC_CORE_LOCAL);

	cpc_redir = mips_cpc_base() + CPC_OFF_REDIRECT;

	stat = __raw_readl(cpc_redir + CPC_Cx_STAT_CONF);
	stat &= CPC_Cx_STAT_CONF_SEQ_STATE;

	if (stat == CPC_Cx_STAT_CONF_SEQ_STATE_U5)
		return 0;

	setup_redirect(cluster, 0, 0, GCR_Cx_REDIRECT_BLOCK_CPC_GLOBAL);

	__raw_writel(CPC_PWRUP_CTL_PWRUP, cpc_redir + CPC_PWRUP_CTL);

	setup_redirect(cluster, GCR_Cx_REDIRECT_CORE_CM_MASTER, 0,
		       GCR_Cx_REDIRECT_BLOCK_CPC_CORE_LOCAL);

	attempts = 200;
	do {
		if (!--attempts)
			return -ETIMEDOUT;

		stat = __raw_readl(cpc_redir + CPC_Cx_STAT_CONF);
		stat &= CPC_Cx_STAT_CONF_SEQ_STATE;
		mdelay(10);
	} while (stat != CPC_Cx_STAT_CONF_SEQ_STATE_U5);

	return 0;
}

int power_down_cluster(unsigned int cluster)
{
	void __iomem *cpc_redir;

	setup_redirect(cluster, 0, 0, GCR_Cx_REDIRECT_BLOCK_CPC_GLOBAL);

	cpc_redir = mips_cpc_base() + CPC_OFF_REDIRECT;
	__raw_writel(0, cpc_redir + CPC_PWRUP_CTL);

	return 0;
}

int init_cluster_l2(unsigned int cluster)
{
	void __iomem *gcr_redir;
	u32 cop, result;

	setup_redirect(cluster, 0, 0, GCR_Cx_REDIRECT_BLOCK_GCR_GLOBAL);

	gcr_redir = mips_cm_base() + GCR_OFF_REDIRECT;

	cop = __raw_readl(gcr_redir + GCR_L2SM_COP);
	if (!(cop & GCR_L2SM_COP_PRESENT))
		return -ENOSYS;

	__raw_writel(0, gcr_redir + GCR_L2_TAG_STATE);
	__raw_writel(0, gcr_redir + GCR_L2_ECC);

	while (cop & GCR_L2SM_COP_RUNNING)
		cop = __raw_readl(gcr_redir + GCR_L2SM_COP);

	cop = GCR_L2SM_COP_TYPE_STORE_TAG | GCR_L2SM_COP_CMD_START;
	__raw_writel(cop, gcr_redir + GCR_L2SM_COP);

	do {
		cop = __raw_readl(gcr_redir + GCR_L2SM_COP);
	} while (cop & GCR_L2SM_COP_RUNNING);

	result = cop & GCR_L2SM_COP_RESULT;

	return (result == GCR_L2SM_COP_RESULT_DONE_OK) ? 0 : -ENXIO;
}

int mips_cm_init(void)
{
	int err;

	if (!(read_c0_config1() & MIPS_CONF_M))
		return 0;
	if (!(read_c0_config2() & MIPS_CONF_M))
		return 0;
	if (!(read_c0_config3() & MIPS_CONF3_CMGCR))
		return 0;

	mips_cpc_init();

	if (CONFIG_IS_ENABLED(MIPS_CM_IOCU)) {
		err = mips_cm_init_iocus();
		if (err)
			return err;
	}

	setup_mmio_limits();

	return 0;
}
