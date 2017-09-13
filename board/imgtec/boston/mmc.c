#include <common.h>
#include <mmc.h>

static struct pci_device_id mmc_supported[] = {
		{ PCI_VENDOR_ID_INTEL, PCI_DEVICE_ID_INTEL_TCF_SDIO_0 },
		{ PCI_VENDOR_ID_INTEL, PCI_DEVICE_ID_INTEL_TCF_SDIO_1 },
		{ }
};

int board_mmc_init(bd_t *bis)
{
	return pci_mmc_init("SDHCI", mmc_supported);
}
