/*
 * Copyright (C) 2015 Imagination Technologies
 * Author: Paul Burton <paul.burton@imgtec.com>
 *
 * SPDX-License-Identifier:	GPL-2.0+
 */

#include <common.h>
#include <asm/maar.h>
#include <asm/system.h>

void mips_maar_init(struct mips_maar_cfg *cfg)
{
	unsigned num_maars, pair, maar;

	if (!cfg)
		return;

	if (!(read_c0_config5() & MIPS_CONF5_MRP))
		return;

	if (getenv_yesno("nomaar") == 1)
		return;

	/* Detect the number of MAARs */
	write_c0_maari(~0);
	execution_hazard_barrier();
	num_maars = read_c0_maari() + 1;

	/* MAARs should be in pairs */
	assert(!(num_maars % 2));

	for (pair = 0; cfg->attrs & MIPS_MAAR_V; cfg++, pair++) {
		write_c0_maari(pair << 1);
		execution_hazard_barrier();
		write_c0_maar(((cfg->upper >> 4) & MIPS_MAAR_ADDR) | cfg->attrs);
		execution_hazard_barrier();

		write_c0_maari((pair << 1) | 0x1);
		execution_hazard_barrier();
		write_c0_maar((cfg->lower >> 4) | cfg->attrs);
		execution_hazard_barrier();
	}

	for (maar = pair * 2; maar < num_maars; maar++) {
		write_c0_maari(maar);
		execution_hazard_barrier();
		write_c0_maar(0);
		execution_hazard_barrier();
	}
}
