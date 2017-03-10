/*
 * Copyright (C) 2016 Imagination Technologies
 * Author: Zubair Lutfullah Kakakhel <Zubair.Kakakhel@imgtec.com>
 *
 * SPDX-License-Identifier:	GPL-2.0+
 */

#include <common.h>
#include <command.h>
#include <asm/byteorder.h>
#include <asm/cache.h>
#include <asm/cm.h>
#include <asm/mipsregs.h>

enum endianness {
	ENDIAN_LITTLE = 0,
	ENDIAN_BIG = 1,
};

static const char *endian_name[2] = {
	[ ENDIAN_LITTLE ] = "little",
	[ ENDIAN_BIG ] = "big",
};

static int switch_cluster_endian(int cluster, enum endianness endianness)
{
	void __iomem *gcrs_global, *gcrs_redir, *cpc_global, *cpc_redir;
	void __iomem *reg_cfg, *sys_cfg;
	unsigned int cm_rev, cfg, num_cores;
	bool have_cpc_sys_config;
	int i;

	gcrs_global = mips_cm_base();
	gcrs_redir = gcrs_global + GCR_OFF_REDIRECT;
	cpc_global = mips_cpc_base();
	cpc_redir = cpc_global + CPC_OFF_REDIRECT;

	cm_rev = __raw_readl(gcrs_global + GCR_REV);
	if (cm_rev < GCR_REV_CM3_5) {
		__raw_writel(endianness, gcrs_global + GCR_SYS_CONFIG);

		/* Make the core non-coherent */
		__raw_writel(0, gcrs_global + GCR_Cx_COHERENCE);

		/* Submit a reset command to the CPC */
		__raw_writel(CPC_Cx_CMD_RESET, cpc_global + CPC_Cx_CMD);

		/* Wait for the reset */
		mdelay(100);

		return -ETIMEDOUT;
	}

	power_up_cluster(cluster);
	setup_redirect(cluster, 0, 0, GCR_Cx_REDIRECT_BLOCK_CPC_GLOBAL);

	have_cpc_sys_config = !!__raw_readl(cpc_redir + CPC_CONFIG);
	if (have_cpc_sys_config) {
		reg_cfg = cpc_redir + CPC_CONFIG;
		sys_cfg = cpc_redir + CPC_SYS_CONFIG;
	} else {
		reg_cfg = gcrs_redir + GCR_CONFIG;
		sys_cfg = gcrs_redir + GCR_SYS_CONFIG;
	}

	__raw_writel(endianness, sys_cfg);
	mb();

	num_cores = (__raw_readl(reg_cfg) + 1) & GCR_CONFIG_NUMCORES_BITS;
	if (num_cores) {
		/*
		 * Reset core 0 in the cluster - at least one core must leave
		 * reset to trigger the CM to switch endianness.
		 */
		setup_redirect(cluster, 0, 0, GCR_Cx_REDIRECT_BLOCK_CPC_CORE_LOCAL);

		/* Make the core non-coherent */
		__raw_writel(0, gcrs_redir + GCR_Cx_COHERENCE - GCR_OFF_LOCAL);
		mb();

		/* Submit a reset command to the clusters CPC */
		__raw_writel(CPC_Cx_CMD_RESET, cpc_redir + CPC_Cx_CMD);
	}

	setup_redirect(cluster, 0, 0, GCR_Cx_REDIRECT_BLOCK_GCR_GLOBAL);

	for (i = 0; i < 100; i++) {
		cfg = swab32(__raw_readl(sys_cfg));
		if (((cfg >> 1) & 0x1) == endianness) {
			power_down_cluster(cluster);
			return 0;
		}

		mdelay(10);
	}

	return -ETIMEDOUT;
}

static int switch_endian(enum endianness new)
{
	int num_clusters = mips_cm_num_clusters();
	int cluster, err;

	if (!(read_c0_config5() & MIPS_CONF5_DEC)) {
		printf("This cpu does not support endian switching\n");
		return 1;
	}

	/* Power up & switch the endianness of secondary clusters */
	for (cluster = num_clusters - 1; cluster > 0; cluster--) {
		printf("Switching cluster %d to %s endian...", cluster, endian_name[new]);
		err = switch_cluster_endian(cluster, new);
		if (err) {
			printf(" failed!\n");

			/* Try to restore endianness */
			for (; cluster < num_clusters; cluster++)
				switch_cluster_endian(cluster, !new);

			return err;
		}
		printf(" done\n");
	}

	/* Disable caches */
	printf("Disabling caches...");
	dcache_disable();
	printf(" done\n");

	/* Now switch the endianness of this cluster */
	printf("Switching cluster 0 to %s endian...", endian_name[new]);
	switch_cluster_endian(0, new);

	/* Give the reset a chance to happen before we complain */
	udelay(10000);

	/* Something went wrong if we reached here */
	printf(" FAILED (Cluster reset didn't occur?)\n");
	return 1;
}

static int do_endian(cmd_tbl_t *cmdtp, int flag, int argc, char * const argv[])
{
	enum endianness curr, new;

#ifdef __BIG_ENDIAN
	curr = ENDIAN_BIG;
#else
	curr = ENDIAN_LITTLE;
#endif

	if (argc < 2) {
		printf("%s\n", endian_name[curr]);
		return 0;
	}

	if (!strcmp(argv[1], "switch"))
		new = !curr;
	else if (!strcmp(argv[1], "big"))
		new = ENDIAN_BIG;
	else if (!strcmp(argv[1], "little"))
		new = ENDIAN_LITTLE;
	else
		return CMD_RET_USAGE;

	if (new == curr) {
		printf("Already %s endian\n", endian_name[curr]);
		return 0;
	}

	return switch_endian(new);
}

U_BOOT_CMD(
	endian,	2, 1, do_endian,
	"view current endianness of cpu or switch endianness or specify switch to endian big/little",
	"[switch, big, little]\n"
);
