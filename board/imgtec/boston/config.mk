#
# SPDX-License-Identifier:	GPL-2.0+
#

quiet_cmd_srec_cat = SRECCAT $@
      cmd_srec_cat = srec_cat -output $@ -$2 $< -binary -offset $3

u-boot.mcs: u-boot.bin
	$(call cmd,srec_cat,intel,0x7c00000)

# if srec_cat is present build u-boot.mcs by default
ifneq ($(shell which srec_cat 2>/dev/null),)
ALL-y += u-boot.mcs
CLEAN_FILES += u-boot.mcs
endif
