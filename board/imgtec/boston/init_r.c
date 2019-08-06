#include <common.h>
#include <pci.h>

int board_early_init_r(void)
{
	pci_init();
	return 0;
}
