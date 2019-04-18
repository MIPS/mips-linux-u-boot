/*
 * Copyright (C) 2015, Google, Inc
 * Copyright (C) 2014, Bin Meng <bmeng.cn@gmail.com>
 *
 * SPDX-License-Identifier:	GPL-2.0+
 */

#include <common.h>
#include <mmc.h>
#include <errno.h>
#include <malloc.h>
#include <pci.h>
#include <sdhci.h>

static struct pci_device_id mmc_supported[] = {
		{ PCI_VENDOR_ID_INTEL, PCI_DEVICE_ID_INTEL_TCF_SDIO_0 },
		{ PCI_VENDOR_ID_INTEL, PCI_DEVICE_ID_INTEL_TCF_SDIO_1 },
		{ }
};

int board_mmc_init(bd_t *bis)
{
	struct sdhci_host *mmc_host;
	int ret;
	int i;

	for (i = 0; ; i++) {
		struct udevice *dev;

		ret = pci_find_device_id(mmc_supported, i, &dev);
		if (ret)
			return ret;
		mmc_host = malloc(sizeof(struct sdhci_host));
		if (!mmc_host)
			return -ENOMEM;

		mmc_host->name = "SDHCI";
		mmc_host->ioaddr = dm_pci_map_bar(dev, PCI_BASE_ADDRESS_0,
						  PCI_REGION_MEM);
		mmc_host->quirks = 0;
		ret = add_sdhci(mmc_host, 0, 0);
		if (ret)
			return ret;
	}

	return 0;
}
