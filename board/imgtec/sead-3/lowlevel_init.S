/*
 * SEAD-3 Board Initialisation
 *
 * Copyright (c) 2017 Imagination Technologies Ltd.
 *
 * SPDX-License-Identifier:	GPL-2.0+
 */

#include <asm/asm.h>
#include <asm/regdef.h>

LEAF(lowlevel_init)
1:	lw	t0, 0xbb100110(zero)
	and	t0, t0, 1 << 5
	bnez	t0, 1b

	jr	ra
	END(lowlevel_init)
