/*
 * MIPS Coherence Manager (CM) Support
 *
 * Copyright (c) 2016 Imagination Technologies Ltd.
 *
 * SPDX-License-Identifier:	GPL-2.0+
 */

#include <common.h>
#include <asm/cm.h>

__weak bool plat_iocu_usable(unsigned int cluster, unsigned int iocu)
{
	return true;
}

static int init_cluster_iocus(unsigned int cluster)
{
	DECLARE_GLOBAL_DATA_PTR;
	void __iomem *global_gcrs;
	u32 cfg, num_iocus, num_iocus_usable, local_cluster;
	int err, i;

	local_cluster = __raw_readl(mips_cm_base() + GCR_Cx_ID);
	local_cluster &= GCR_Cx_ID_CLUSTER_BITS;
	local_cluster >>= GCR_Cx_ID_CLUSTER_SHIFT;

	global_gcrs = mips_cm_base();
	if (cluster != local_cluster) {
		/*
		 * We're examining a remote cluster. Before we can access its
		 * CM GCRs to check for IOCUs we need to ensure that the CM
		 * is powered up.
		 */
		err = power_up_cluster(cluster);
		if (err)
			return err;

		setup_redirect(cluster, 0, 0, GCR_Cx_REDIRECT_BLOCK_GCR_GLOBAL);
		global_gcrs += GCR_OFF_REDIRECT;
	}

	cfg = __raw_readl(global_gcrs + GCR_CONFIG);
	num_iocus = cfg & GCR_CONFIG_NUMIOCU_BITS;
	num_iocus >>= GCR_CONFIG_NUMIOCU_SHIFT;
	gd->arch.num_iocus += num_iocus;

	/* Discover how many IOCUs are usable */
	num_iocus_usable = num_iocus;
	for (i = num_iocus - 1; i >= 0; i--) {
		if (!plat_iocu_usable(cluster, i))
			num_iocus_usable--;
	}
	gd->arch.num_iocus_usable += num_iocus_usable;

	/* If the cluster has no usable IOCUs there's nothing to do */
	if (num_iocus_usable == 0) {
		if (cluster != local_cluster)
			power_down_cluster(cluster);

		return 0;
	}

	/* If the IOCUs are in the local cluster we're good to go already */
	if (cluster == local_cluster)
		return 0;

	/* Ensure that the cluster's L2 cache is initialised */
	return init_cluster_l2(cluster);
}

int mips_cm_init_iocus(void)
{
	DECLARE_GLOBAL_DATA_PTR;
	unsigned int cluster;
	int err;

	for (cluster = 0; cluster < mips_cm_num_clusters(); cluster++) {
		err = init_cluster_iocus(cluster);
		if (err) {
			gd->arch.num_iocus_usable = err;
			return 0;
		}
	}

	return 0;
}
