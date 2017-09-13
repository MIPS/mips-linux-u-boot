/*
 * Copyright (c) 2015 Google, Inc
 * Written by Simon Glass <sjg@chromium.org>
 *
 * SMBus block read/write support added by Stefan Roese:
 * Copyright (C) 2016 Stefan Roese <sr@denx.de>
 *
 * SPDX-License-Identifier:     GPL-2.0+
 */

#ifndef __INTEL_I2C_H__
#define __INTEL_I2C_H__

/* SMBus I/O bits. */
#define SMBHSTSTAT		0x0
#define SMBHSTCTL		0x2
#define SMBHSTCMD		0x3
#define SMBXMITADD		0x4
#define SMBHSTDAT0		0x5
#define SMBHSTDAT1		0x6
#define SMBBLKDAT		0x7
#define SMBTRNSADD		0x9
#define SMBSLVDATA		0xa
#define SMBAUXCTL		0xd
#define SMLINK_PIN_CTL		0xe
#define SMBUS_PIN_CTL		0xf

/* I801 Hosts Status register bits */
#define SMBHSTSTS_BYTE_DONE	0x80
#define SMBHSTSTS_INUSE_STS	0x40
#define SMBHSTSTS_SMBALERT_STS	0x20
#define SMBHSTSTS_FAILED	0x10
#define SMBHSTSTS_BUS_ERR	0x08
#define SMBHSTSTS_DEV_ERR	0x04
#define SMBHSTSTS_INTR		0x02
#define SMBHSTSTS_HOST_BUSY	0x01

/* I801 Host Control register bits */
#define SMBHSTCNT_INTREN	0x01
#define SMBHSTCNT_KILL		0x02
#define SMBHSTCNT_CMD_BYTE_DATA	(0x2 << 2)
#define SMBHSTCNT_LAST_BYTE	0x20
#define SMBHSTCNT_START		0x40
#define SMBHSTCNT_PEC_EN	0x80	/* ICH3 and later */

/* Address register bits */
#define SMBXMITADD_WRITE	0x0
#define SMBXMITADD_READ		0x1
#define SMBXMITADD_ADDR_SHF	1

/* Auxiliary control register bits, ICH4+ only */
#define SMBAUXCTL_CRC		1
#define SMBAUXCTL_E32B		2

#endif /* __INTEL_I2C_H__ */
