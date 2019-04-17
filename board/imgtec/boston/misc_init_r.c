/*
 * Copyright (C) 2016 Imagination Technologies
 *
 * SPDX-License-Identifier:	GPL-2.0
 */

#include <common.h>
#include <environment.h>
#include <net.h>

int misc_init_r(void)
{
	/* Generate a random MAC if one isn't set */
	if (!env_get("ethaddr")) {
		uchar addr[6];
		net_random_ethaddr(addr);
		eth_env_set_enetaddr("ethaddr", addr);
	}

	return 0;
}
