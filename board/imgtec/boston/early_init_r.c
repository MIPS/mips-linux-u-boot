#include <common.h>
#include <linux/sizes.h>

int board_early_init_r(void)
{
	DECLARE_GLOBAL_DATA_PTR;

	gd->pci_ram_top = SZ_2G;
	return 0;
}
