#
# (C) Copyright 2003
# Wolfgang Denk, DENX Software Engineering, wd@denx.de.
#
# SPDX-License-Identifier:	GPL-2.0+
#

bits-$(CONFIG_32BIT)		:= 32
bits-$(CONFIG_64BIT)		:= 64

end-$(CONFIG_SYS_BIG_ENDIAN)	:= big
end-$(CONFIG_SYS_LITTLE_ENDIAN)	:= little
end-letter			:= $(firstword $(subst i, i,$(end-y)))
end-letter-upper		:= $(subst b,B,$(subst l,L,$(end-letter)))

isa-$(CONFIG_ISA_MIPS)		:= mips
isa-$(CONFIG_ISA_NANOMIPS)	:= nanomips

cppflags-$(CONFIG_ISA_MIPS)	+= -mabi=$(bits-y) -mno-abicalls
cppflags-$(CONFIG_ISA_NANOMIPS)	+= -m$(bits-y)
PLATFORM_CPPFLAGS		+= $(cppflags-y) -E$(end-letter-upper) -G 0 -fno-pic
PLATFORM_CPPFLAGS		+= -msoft-float
PLATFORM_CPPFLAGS		+= -D__MIPS__

ldflags-$(CONFIG_ISA_MIPS)	+= -m elf$(bits-y)$(end-letter)tsmip
ldflags-$(CONFIG_ISA_MIPS)	+= -G 0
PLATFORM_LDFLAGS		+= $(ldflags-y) -E$(end-letter-upper) -static -n -nostdlib

PLATFORM_RELFLAGS		+= -ffunction-sections -fdata-sections

#LDFLAGS_FINAL			+= --gc-sections

OBJCOPYFLAGS			+= -O elf$(bits-y)-trad$(end-y)$(isa-y)
OBJCOPYFLAGS			+= -j .text -j .rodata -j .data -j .u_boot_list

ifndef CONFIG_SPL_BUILD
OBJCOPYFLAGS			+= -j .got -j .rel -j .padding -j .dtb.init.rodata
LDFLAGS_FINAL			+= --emit-relocs
endif

PLATFORM_ELFENTRY = "__start"
PLATFORM_ELFFLAGS += -B mips $(OBJCOPYFLAGS)
