/*
 * Copyright (C) 2015 Imagination Technologies
 *
 * SPDX-License-Identifier:	GPL-2.0
 */

#include <config.h>

#include <asm/addrspace.h>
#include <asm/asm.h>
#include <asm/mipsregs.h>
#include <asm/regdef.h>

.text

LEAF(lowlevel_init)
	li	t0, CKSEG1ADDR(0x1b100110)
1:	lw	t1, 0(t0)
	andi	t1, t1, 1 << 5
	bnez	t1, 1b

	move	v0, zero
	jr	ra
	END(lowlevel_init)

LEAF(board_nmi_dispatch)
	PTR_LI	k1, CKSEG0ADDR(0x380)
	jr	k1
	END(board_nmi_dispatch)

LEAF(board_ejtag_dispatch)
	PTR_LI	k1, CKSEG0ADDR(0x300)
	jr	k1
	END(board_ejtag_dispatch)
