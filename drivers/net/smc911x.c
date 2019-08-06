/*
 * SMSC LAN9[12]1[567] Network driver
 *
 * (c) 2007 Pengutronix, Sascha Hauer <s.hauer@pengutronix.de>
 *
 * SPDX-License-Identifier:	GPL-2.0+
 */

#include <common.h>
#include <command.h>
#include <dm.h>
#include <malloc.h>
#include <net.h>
#include <miiphy.h>
#include <asm/io.h>

#include "smc911x.h"

u32 pkt_data_pull(struct smc911x_priv *dev, u32 addr) \
	__attribute__ ((weak, alias ("smc911x_reg_read")));
void pkt_data_push(struct smc911x_priv *dev, u32 addr, u32 val) \
	__attribute__ ((weak, alias ("smc911x_reg_write")));

static u32 __smc911x_reg_read(struct smc911x_priv *priv, u32 offset)
{
	unsigned int io_width;
	void __iomem *base;
	u32 ret;

#if defined(CONFIG_DM_ETH)
	struct eth_pdata *pdata = dev_get_platdata(priv->dev);

	base = (void __iomem *)pdata->iobase;
	io_width = priv->io_width;
#elif defined (CONFIG_SMC911X_32_BIT)
	base = (void __iomem *)priv->eth.iobase;
	io_width = 4;
#elif defined(CONFIG_SMC911X_16_BIT)
	base = (void __iomem *)priv->eth.iobase;
	io_width = 2;
#else
# error "SMC911X: undefined bus width"
#endif

	switch (io_width) {
	case 2:
		ret = __raw_readw(base + offset);
		ret |= __raw_readw(base + offset + 2) << 16;
		return ret;

	case 4:
		return __raw_readl(base + offset);

	default:
		panic("Invalid io_width\n");
		return 0;
	}
}

static inline void __smc911x_reg_write(struct smc911x_priv *priv,
					u32 offset, u32 val)
{
	unsigned int io_width;
	void __iomem *base;

#if defined(CONFIG_DM_ETH)
	struct eth_pdata *pdata = dev_get_platdata(priv->dev);

	base = (void __iomem *)pdata->iobase;
	io_width = priv->io_width;
#elif defined (CONFIG_SMC911X_32_BIT)
	base = (void __iomem *)priv->eth.iobase;
	io_width = 4;
#elif defined(CONFIG_SMC911X_16_BIT)
	base = (void __iomem *)priv->eth.iobase;
	io_width = 2;
#else
# error "SMC911X: undefined bus width"
#endif

	switch (io_width) {
	case 2:
		__raw_writew(val, base + offset);
		__raw_writew(val >> 16, base + offset + 2);
		break;

	case 4:
		__raw_writel(val, base + offset);
		break;

	default:
		panic("Invalid io_width\n");
	}
}

static uchar *smc911x_mac_ptr(struct smc911x_priv *dev)
{
#ifdef CONFIG_DM_ETH
	struct eth_pdata *pdata = dev_get_platdata(dev->dev);

	return pdata->enetaddr;
#else
	return dev->eth.enetaddr;
#endif
}

static void smc911x_probe_mac(struct smc911x_priv *dev)
{
	uchar *m = smc911x_mac_ptr(dev);
	unsigned long addrh, addrl;

	addrh = smc911x_get_mac_csr(dev, ADDRH);
	addrl = smc911x_get_mac_csr(dev, ADDRL);

	/* address is obtained from optional eeprom */
	if (addrl == 0xffffffff && addrh == 0x0000ffff)
		return;

	m[0] = addrl;
	m[1] = addrl >>  8;
	m[2] = addrl >> 16;
	m[3] = addrl >> 24;
	m[4] = addrh;
	m[5] = addrh >> 8;
}

static void smc911x_handle_mac_address(struct smc911x_priv *dev)
{
	uchar *m = smc911x_mac_ptr(dev);
	unsigned long addrh, addrl;

	addrl = m[0] | (m[1] << 8) | (m[2] << 16) | (m[3] << 24);
	addrh = m[4] | (m[5] << 8);
	smc911x_set_mac_csr(dev, ADDRL, addrl);
	smc911x_set_mac_csr(dev, ADDRH, addrh);

	printf(DRIVERNAME ": MAC %pM\n", m);
}

static int smc911x_eth_phy_read(struct smc911x_priv *dev,
				u8 phy, u8 reg, u16 *val)
{
	while (smc911x_get_mac_csr(dev, MII_ACC) & MII_ACC_MII_BUSY)
		;

	smc911x_set_mac_csr(dev, MII_ACC, phy << 11 | reg << 6 |
				MII_ACC_MII_BUSY);

	while (smc911x_get_mac_csr(dev, MII_ACC) & MII_ACC_MII_BUSY)
		;

	*val = smc911x_get_mac_csr(dev, MII_DATA);

	return 0;
}

static int smc911x_eth_phy_write(struct smc911x_priv *dev,
				u8 phy, u8 reg, u16  val)
{
	while (smc911x_get_mac_csr(dev, MII_ACC) & MII_ACC_MII_BUSY)
		;

	smc911x_set_mac_csr(dev, MII_DATA, val);
	smc911x_set_mac_csr(dev, MII_ACC,
		phy << 11 | reg << 6 | MII_ACC_MII_BUSY | MII_ACC_MII_WRITE);

	while (smc911x_get_mac_csr(dev, MII_ACC) & MII_ACC_MII_BUSY)
		;
	return 0;
}

static int smc911x_phy_reset(struct smc911x_priv *dev)
{
	u32 reg;

	reg = smc911x_reg_read(dev, PMT_CTRL);
	reg &= ~0xfffff030;
	reg |= PMT_CTRL_PHY_RST;
	smc911x_reg_write(dev, PMT_CTRL, reg);

	mdelay(100);

	return 0;
}

static void smc911x_phy_configure(struct smc911x_priv *dev)
{
	int timeout;
	u16 status;

	smc911x_phy_reset(dev);

	smc911x_eth_phy_write(dev, 1, MII_BMCR, BMCR_RESET);
	mdelay(1);
	smc911x_eth_phy_write(dev, 1, MII_ADVERTISE, 0x01e1);
	smc911x_eth_phy_write(dev, 1, MII_BMCR, BMCR_ANENABLE |
				BMCR_ANRESTART);

	timeout = 5000;
	do {
		mdelay(1);
		if ((timeout--) == 0)
			goto err_out;

		if (smc911x_eth_phy_read(dev, 1, MII_BMSR, &status) != 0)
			goto err_out;
	} while (!(status & BMSR_LSTATUS));

	printf(DRIVERNAME ": phy initialized\n");

	return;

err_out:
	printf(DRIVERNAME ": autonegotiation timed out\n");
}

static void smc911x_enable(struct smc911x_priv *dev)
{
	/* Enable TX */
	smc911x_reg_write(dev, HW_CFG, 8 << 16 | HW_CFG_SF);

	smc911x_reg_write(dev, GPT_CFG, GPT_CFG_TIMER_EN | 10000);

	smc911x_reg_write(dev, TX_CFG, TX_CFG_TX_ON);

	/* no padding to start of packets */
	smc911x_reg_write(dev, RX_CFG, 0);

	smc911x_set_mac_csr(dev, MAC_CR, MAC_CR_TXEN | MAC_CR_RXEN |
				MAC_CR_HBDIS);

}

static int __smc911x_start(struct smc911x_priv *dev)
{
	printf(DRIVERNAME ": detected %s controller\n", dev->chip_id->name);

	smc911x_reset(dev);

	/* Configure the PHY, initialize the link state */
	smc911x_phy_configure(dev);

	smc911x_handle_mac_address(dev);

	/* Turn on Tx + Rx */
	smc911x_enable(dev);

	return 0;
}

static int __smc911x_send(struct smc911x_priv *dev, void *packet, int length)
{
	u32 *data = (u32*)packet;
	u32 tmplen;
	u32 status;

	smc911x_reg_write(dev, TX_DATA_FIFO, TX_CMD_A_INT_FIRST_SEG |
				TX_CMD_A_INT_LAST_SEG | length);
	smc911x_reg_write(dev, TX_DATA_FIFO, length);

	tmplen = (length + 3) / 4;

	while (tmplen--)
		pkt_data_push(dev, TX_DATA_FIFO, *data++);

	/* wait for transmission */
	while (!((smc911x_reg_read(dev, TX_FIFO_INF) &
					TX_FIFO_INF_TSUSED) >> 16));

	/* get status. Ignore 'no carrier' error, it has no meaning for
	 * full duplex operation
	 */
	status = smc911x_reg_read(dev, TX_STATUS_FIFO) &
			(TX_STS_LOC | TX_STS_LATE_COLL | TX_STS_MANY_COLL |
			TX_STS_MANY_DEFER | TX_STS_UNDERRUN);

	if (!status)
		return 0;

	printf(DRIVERNAME ": failed to send packet: %s%s%s%s%s\n",
		status & TX_STS_LOC ? "TX_STS_LOC " : "",
		status & TX_STS_LATE_COLL ? "TX_STS_LATE_COLL " : "",
		status & TX_STS_MANY_COLL ? "TX_STS_MANY_COLL " : "",
		status & TX_STS_MANY_DEFER ? "TX_STS_MANY_DEFER " : "",
		status & TX_STS_UNDERRUN ? "TX_STS_UNDERRUN" : "");

	return -1;
}

static void __smc911x_stop(struct smc911x_priv *dev)
{
	smc911x_reset(dev);
	smc911x_handle_mac_address(dev);
}

static int __smc911x_recv(struct smc911x_priv *dev, int flags, uchar **packetp)
{
	u32 *data = (u32 *)net_rx_packets[0];
	u32 pktlen, tmplen;
	u32 status;

	if ((smc911x_reg_read(dev, RX_FIFO_INF) & RX_FIFO_INF_RXSUSED) >> 16) {
		status = smc911x_reg_read(dev, RX_STATUS_FIFO);
		pktlen = (status & RX_STS_PKT_LEN) >> 16;

		smc911x_reg_write(dev, RX_CFG, 0);

		*packetp = (void *)data;

		tmplen = (pktlen + 3) / 4;
		while (tmplen--)
			*data++ = pkt_data_pull(dev, RX_DATA_FIFO);

		if (status & RX_STS_ES)
			printf(DRIVERNAME
				": dropped bad packet. Status: 0x%08x\n",
				status);

		return pktlen;
	}

	return 0;
}

#if defined(CONFIG_MII) || defined(CONFIG_CMD_MII)
/* wrapper for smc911x_eth_phy_read */
static int smc911x_miiphy_read(struct mii_dev *bus, int phy, int devad,
			       int reg)
{
	u16 val = 0;
	struct eth_device *dev = eth_get_dev_by_name(bus->name);
	if (dev) {
		int retval = smc911x_eth_phy_read(dev, phy, reg, &val);
		if (retval < 0)
			return retval;
		return val;
	}
	return -ENODEV;
}
/* wrapper for smc911x_eth_phy_write */
static int smc911x_miiphy_write(struct mii_dev *bus, int phy, int devad,
				int reg, u16 val)
{
	struct eth_device *dev = eth_get_dev_by_name(bus->name);
	if (dev)
		return smc911x_eth_phy_write(dev, phy, reg, val);
	return -ENODEV;
}
#endif

#ifdef CONFIG_DM_ETH

static int smc911x_start(struct udevice *dev)
{
	struct smc911x_priv *priv = dev_get_priv(dev);

	return __smc911x_start(priv);
}

static int smc911x_send(struct udevice *dev, void *packet, int length)
{
	struct smc911x_priv *priv = dev_get_priv(dev);

	return __smc911x_send(priv, packet, length);
}

static int smc911x_recv(struct udevice *dev, int flags, uchar **packetp)
{
	struct smc911x_priv *priv = dev_get_priv(dev);

	return __smc911x_recv(priv, flags, packetp);
}

static void smc911x_stop(struct udevice *dev)
{
	struct smc911x_priv *priv = dev_get_priv(dev);

	__smc911x_stop(priv);
}

static const struct eth_ops smc911x_ops = {
	.start		= smc911x_start,
	.send		= smc911x_send,
	.recv		= smc911x_recv,
	.stop		= smc911x_stop,
};

int smc911x_ofdata_to_platdata(struct udevice *dev)
{
	struct eth_pdata *pdata = dev_get_platdata(dev);
	struct smc911x_priv *priv = dev_get_priv(dev);
	fdt_addr_t addr;

	addr = dev_read_addr(dev);
	if (addr == FDT_ADDR_T_NONE)
		return -EINVAL;

	pdata->iobase = (phys_addr_t)map_physmem(addr, 0, MAP_NOCACHE);

	priv->dev = dev;
	priv->io_width = dev_read_u32_default(dev, "reg-io-width", 2);

	return 0;
}

static int smc911x_probe(struct udevice *dev)
{
	struct smc911x_priv *priv = dev_get_priv(dev);

	if (smc911x_detect_chip(priv))
		return -ENODEV;

	smc911x_probe_mac(priv);

	return 0;
}

static int smc911x_remove(struct udevice *dev)
{
	return 0;
}

static const struct udevice_id smc911x_ids[] = {
	{ .compatible = "smsc,lan9115" },
	{ }
};

U_BOOT_DRIVER(smc911x) = {
	.name			= "smc911x",
	.id			= UCLASS_ETH,
	.of_match		= smc911x_ids,
	.ofdata_to_platdata	= smc911x_ofdata_to_platdata,
	.probe			= smc911x_probe,
	.remove			= smc911x_remove,
	.ops			= &smc911x_ops,
	.priv_auto_alloc_size	= sizeof(struct smc911x_priv),
	.platdata_auto_alloc_size	= sizeof(struct eth_pdata),
};

#else /* !CONFIG_DM_ETH */

static int smc911x_init(struct eth_device *dev, bd_t * bd)
{
	struct smc911x_priv *priv = container_of(dev, struct smc911x_priv, eth);

	return __smc911x_start(priv);
}

static void smc911x_halt(struct eth_device *dev)
{
	struct smc911x_priv *priv = container_of(dev, struct smc911x_priv, eth);

	__smc911x_stop(priv);
}

static int smc911x_send(struct eth_device *dev, void *packet, int length)
{
	struct smc911x_priv *priv = container_of(dev, struct smc911x_priv, eth);

	return __smc911x_send(priv, packet, length);
}

static int smc911x_rx(struct eth_device *dev)
{
	struct smc911x_priv *priv = container_of(dev, struct smc911x_priv, eth);
	uchar *packetp;
	int len;

	__smc911x_rx(priv, 0, &packetp);
	if (len)
		net_process_received_packet(packetp, len);

	return 0;
}

int smc911x_initialize(u8 dev_num, int base_addr)
{
	struct smc911x_priv *priv;
	struct eth_device *dev;

	priv = malloc(sizeof(*priv));
	if (!priv) {
		return -1;
	}
	memset(priv, 0, sizeof(*priv));

	dev = &priv->dev;
	dev->iobase = base_addr;

	/* Try to detect chip. Will fail if not present. */
	if (smc911x_detect_chip(priv)) {
		free(dev);
		return 0;
	}

	smc911x_probe_mac(priv);

	dev->init = smc911x_init;
	dev->halt = smc911x_halt;
	dev->send = smc911x_send;
	dev->recv = smc911x_rx;
	sprintf(dev->name, "%s-%hu", DRIVERNAME, dev_num);

	eth_register(dev);

#if defined(CONFIG_MII) || defined(CONFIG_CMD_MII)
	int retval;
	struct mii_dev *mdiodev = mdio_alloc();
	if (!mdiodev)
		return -ENOMEM;
	strncpy(mdiodev->name, dev->name, MDIO_NAME_LEN);
	mdiodev->read = smc911x_miiphy_read;
	mdiodev->write = smc911x_miiphy_write;

	retval = mdio_register(mdiodev);
	if (retval < 0)
		return retval;
#endif

	return 1;
}

#endif /* !CONFIG_DM_ETH */
