#
# SPDX-License-Identifier:	GPL-2.0+
#

ifeq ("$(origin V)", "command line")
  KBUILD_VERBOSE = $(V)
endif
ifndef KBUILD_VERBOSE
  KBUILD_VERBOSE = 0
endif
ifeq ($(KBUILD_VERBOSE),1)
  Q =
else
  Q = @
endif

srctree := ../../..
objtree ?= $(srctree)

ifeq ($(FAT_BOARD),)
$(error Please specify FAT_BOARD=)
endif

include $(srctree)/board/$(FAT_BOARD)/fat.mk

ifeq ("$(origin BUILDS)", "command line")
  FAT_BUILDS := $(BUILDS)
endif
ifeq ($(FAT_BUILDS),)
$(error No FAT_BUILDS specified for the board)
endif

fat_dirs := $(addprefix $(objtree)/.fat/,$(FAT_BUILDS))
fat_elfs := $(addsuffix /u-boot,$(fat_dirs))

.PHONY: fat
fat: u-boot.bin

u-boot.mcs: u-boot.bin
	$(Q)srec_cat -output $@ -intel $< -binary -offset 0x7c00000

u-boot.fl: u-boot.bin
	$(Q)$(srctree)/tools/flgen.py $< $@

u-boot.bin: $(fat_elfs) gen-fat-binary
	$(Q)./gen-fat-binary -o $@ $(addprefix -b ,$(FAT_BASE)) $(fat_elfs)

$(fat_elfs): outdir = $(dir $@)
$(fat_elfs): build = $(notdir $(patsubst %/,%,$(outdir)))
$(fat_elfs): defconfig = $(build)_defconfig
$(fat_elfs): text_base = $(FAT_BASE_$(build))
$(fat_elfs):
	$(Q)mkdir -p $(srctree)/kernel/configs
	$(Q)echo "CONFIG_SYS_TEXT_BASE=$(text_base)" >$(srctree)/kernel/configs/$(build)-fat.config
	$(Q)$(MAKE) -C $(srctree) \
		O=$(abspath $(outdir)) \
		V=$(KBUILD_VERBOSE) \
		$(defconfig) $(build)-fat.config u-boot

.PHONY: clean
clean:
	$(Q)rm -r $(objtree)/.fat
