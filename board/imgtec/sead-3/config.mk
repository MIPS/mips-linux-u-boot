#
# SEAD-3 Board Support
#
# Copyright (c) 2017 Imagination Technologies Ltd.
#
# SPDX-License-Identifier:	GPL-2.0+
#

ALL-y += u-boot.fl

quiet_cmd_mips_fl = FL      $@
cmd_mips_fl = $(srctree)/tools/mips-fl-gen.py $< $@

u-boot.fl: u-boot.bin
	$(call if_changed,mips_fl)
