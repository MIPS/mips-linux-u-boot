
#include <common.h>
#include <errno.h>
#include <i2c.h>
#include <pci.h>

#include <asm/io.h>

static u32 smbus_io_base;

#define MAKE_REG_ACCESSORS(name, addr)			\
static inline uchar read_##name(void)			\
{							\
	return inb(smbus_io_base + addr);		\
}							\
							\
static inline void write_##name(uchar data)		\
{							\
	outb(data, smbus_io_base + addr);		\
}

MAKE_REG_ACCESSORS(sts, 0)
MAKE_REG_ACCESSORS(cnt, 2)
MAKE_REG_ACCESSORS(cmd, 3)
MAKE_REG_ACCESSORS(add, 4)
MAKE_REG_ACCESSORS(dat0, 5)
MAKE_REG_ACCESSORS(blkdat, 7)

#define STS_BUSY		(1 << 0)
#define STS_INTER		(1 << 1)

#define CNT_KILL		(1 << 1)
#define CNT_CMD_SHIFT		2
#define CNT_CMD_BYTE		0x1
#define CNT_CMD_BYTE_DATA	0x2
#define CNT_CMD_BLOCK		0x5
#define CNT_START		(1 << 6)

#define ADD_READ		(1 << 0)
#define ADD_ADDR_SHIFT		1

static void piix4_i2c_init(struct i2c_adapter *adap, int speed, int slaveaddr)
{
	pci_dev_t bdf;
	DECLARE_GLOBAL_DATA_PTR;

	if (!(gd->flags & GD_FLG_RELOC))
		return;

	bdf = pci_find_device(PCI_VENDOR_ID_INTEL,
			      PCI_DEVICE_ID_INTEL_82371AB_3, 0);
	if (bdf == -1)
		panic("Failed to find PIIX4 smbus controller\n");

	/* enable SMBus host controller */
	pci_write_config_byte(bdf, 0xd2, 0x01);

	/* enable SMBus I/O space */
	pci_write_config_word(bdf, 0x04, 0x0001);

	/* find I/O space */
	pci_read_config_dword(bdf, 0x90, &smbus_io_base);
	smbus_io_base &= PCI_BASE_ADDRESS_IO_MASK;

	write_cnt(CNT_KILL);
}

static unsigned int piix4_i2c_set_bus_speed(struct i2c_adapter *adap,
					    unsigned int speed)
{
	return 0;
}

static int piix4_wait_ready(void)
{
	int timeout = 10000;
	uchar sts;

	sts = read_sts();
	while (sts) {
		write_sts(sts);

		if (!--timeout) {
			printf("%s timeout! sts=0x%02x\n", __func__, sts);
			return -ETIMEDOUT;
		}

		sts = read_sts();
	}

	return 0;
}

static int piix4_wait_done(void)
{
	uchar sts;
	int timeout = 10000;

	sts = read_sts();
	while ((sts & (STS_BUSY | STS_INTER)) != STS_INTER) {
		sts = read_sts();

		if (--timeout)
			continue;

		printf("%s timeout! sts=0x%02x\n", __func__, sts);
		return -ETIMEDOUT;
	}

	return 0;
}

static int piix4_i2c_read_byte_data(uchar addr, uchar cmd, uchar *data)
{
	int err;

	write_add((addr << ADD_ADDR_SHIFT) | ADD_READ);
	write_cmd(cmd);

	err = piix4_wait_ready();
	if (err)
		return err;

	write_cnt(CNT_START | (CNT_CMD_BYTE_DATA << CNT_CMD_SHIFT));

	err = piix4_wait_done();
	if (err)
		return err;

	*data = read_dat0();
	return 0;
}

static int piix4_i2c_write_byte(uchar addr, uchar data)
{
	int err;

	write_add(addr << ADD_ADDR_SHIFT);

	err = piix4_wait_ready();
	if (err)
		return err;

	write_cnt(CNT_START | (CNT_CMD_BYTE << CNT_CMD_SHIFT));
	return piix4_wait_done();
}

static int piix4_i2c_probe(struct i2c_adapter *adap, uchar chip)
{
	return piix4_i2c_write_byte(chip, 0);
}

static int piix4_i2c_read(struct i2c_adapter *adap, uchar chip, uint addr,
			  int alen, uchar *buffer, int len)
{
	int i, err;

	/* only support 7 bit addresses */
	if (alen != 1)
		return -ENXIO;

	for (i = 0; i < len; i++) {
		err = piix4_i2c_read_byte_data(chip, addr + i, &buffer[i]);
		if (err)
			return err;
	}

	return 0;
}

static int piix4_i2c_write(struct i2c_adapter *adap, uchar chip, uint addr,
			   int alen, uchar *buffer, int len)
{
	/* only support 7 bit addresses */
	if (alen != 1)
		return -ENXIO;

	/* TODO */
	return -EIO;
}

U_BOOT_I2C_ADAP_COMPLETE(smbus, piix4_i2c_init, piix4_i2c_probe,
			 piix4_i2c_read, piix4_i2c_write,
			 piix4_i2c_set_bus_speed, 100000, 0, 0)
