# SPDX-License-Identifier: GPL-2.0+

extra-y	= start.o

obj-y += time.o
obj-y += interrupts.o
obj-y += cpu.o

obj-$(CONFIG_MIPS_CM)		+= cm_init.o cm.o
obj-$(CONFIG_MIPS_CM_IOCU)	+= cm-iocu.o

obj-$(CONFIG_CPU)	+= cpu-driver.o
