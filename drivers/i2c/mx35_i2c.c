/*
 * mx35_i2c.c 
 *
 * Copyright 2010 Amazon Technologies, Inc. All Rights Reserved.
 *
 * The code contained herein is licensed under the GNU General Public
 * License. You may obtain a copy of the GNU General Public License
 * Version 2 or later at the following locations:
 *
 * http://www.opensource.org/licenses/gpl-license.html
 * http://www.gnu.org/copyleft/gpl.html
 */

#include <common.h>
#include <command.h>

#if CONFIG_MX35_I2C

#include "mx35_i2c.h"

#define DPRINTF(args...)
//#define DPRINTF(args...)  printf(args)

//static u32 i2c_bus_num = 0;
static u32 i2c_bus_num_mux = 0;
static struct imx35_i2c_base i2c_base[] = {{0, 0, 0, 0, 0, 0, 0},
			{0x53F80030, 0x00000c00, I2C1_IADR, I2C1_IFDR, I2C1_I2CR, I2C1_I2SR, I2C1_I2DR},	//CONFIG_I2C_BUS1
			{0x53F80030, 0x00003000, I2C2_IADR, I2C2_IFDR, I2C2_I2CR, I2C2_I2SR, I2C2_I2DR},	//CONFIG_I2C_BUS2
						      {0, 0, 0, 0, 0, 0, 0}};

static struct imx35_i2c_dev i2c_device[] = { {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},		//bus #0, not used
#ifdef CONFIG_I2C_AUDIO_CODEC
						   {0x1A, 0x01, 0, 0, 0, 0, 0, 0, 0, 0, 0},	//bus #1, i2c2, predefined
#endif
#ifdef CONFIG_I2C_SMART_BATTERY
						   {0x55, 0x01, 0, 0, 0, 0, 0, 0, 0, 0, 0},	//bus #2, i2c1, predefined
#endif
#ifdef CONFIG_I2C_ACCELEROMETER
						   {0x18, 0x01, 0, 0, 0, 0, 0, 0, 0, 0, 0},	//bus #3, i2c1, predefined
#endif
#ifdef CONFIG_I2C_TEMPERATURE
						   {0x48, 0x01, 0, 0, 0, 0, 0, 0, 0, 0, 0},	//bus #3, i2c2, predefined
#endif
						   {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0}};		//bus #n, not used
struct imx35_i2c_dev *i2c_dev=&i2c_device[0];

void i2c_init_board(void)
{
}

static int i2x_mux_select_mux(int bus)
{
	return 0;	//success
}

unsigned int i2c_get_bus_num(void)
{
	return i2c_bus_num_mux;
}

int i2c_set_bus_num(unsigned int bus)
{
	int	ret;

	if ((bus < 1) || (bus > 3))			//max bus number is 3, Jun, 3, 2010
		return -1;

	int old_bus = i2c_get_bus_num();
	if ((old_bus == bus) && (old_bus > 0))	//already set
		return 0;
	else
		i2c_dev = &i2c_device[bus];

	if (i2c_dev->i2c_comm == 0)
	{
#ifdef CONFIG_I2C_AUDIO_CODEC
		if (bus == I2C_BUS_AUDIO_CODEC)
		{
			DPRINTF("\ninit AUDIO_CODEC bus [%02x]!!!\n", bus);
			i2c_dev->i2c_comm = &i2c_base[CONFIG_I2C_BUS2];
		}
#endif
#ifdef CONFIG_I2C_SMART_BATTERY
		if (bus == I2C_BUS_SMART_BATTERY)
		{
			DPRINTF("\ninit SMART_BATTERY bus [%02x]!!!\n", bus);
			i2c_dev->i2c_comm = &i2c_base[CONFIG_I2C_BUS1];
		}
#endif
#ifdef CONFIG_I2C_ACCELEROMETER
		if (bus == I2C_BUS_ACCELEROMETER)
		{
			DPRINTF("\ninit ACCELEROMETER bus [%02x]!!!\n", bus);
			i2c_dev->i2c_comm = &i2c_base[CONFIG_I2C_BUS1];
		}
#endif
#ifdef CONFIG_I2C_TEMPERATURE
		if (bus == I2C_BUS_TEMPERATURE)
		{
			DPRINTF("\ninit ACCELEROMETER bus [%02x]!!!\n", bus);
			i2c_dev->i2c_comm = &i2c_base[CONFIG_I2C_BUS2];
		}
#endif
	}

	DPRINTF("i2c_dev->chip [0x%02x]\n", i2c_dev->chip);
	DPRINTF("i2c_dev->alen [0x%02x]\n", i2c_dev->alen);

	DPRINTF("set bus [%02x]!!!\n", bus);
	ret = i2x_mux_select_mux(bus);
	if (ret)
	{
		return ret;
	}
	i2c_bus_num_mux = bus;

	i2c_init(0x00, 0x00);

	return 0;	//success
}

void i2c_init(int speed, int slaveaddr)
{
	DPRINTF("init i2c_dev->i2c_comm->base_clock [0x%08x]\n", i2c_dev->i2c_comm->base_clock);
	DPRINTF("init i2c_dev->i2c_comm->data_clock [0x%08x]\n", i2c_dev->i2c_comm->data_clock);
	DPRINTF("init i2c_dev->i2c_comm->base_i2cr [0x%08x]\n", i2c_dev->i2c_comm->base_i2cr);
	DPRINTF("init i2c_dev->i2c_comm->base_ifdr [0x%08x]\n", i2c_dev->i2c_comm->base_ifdr);
	DPRINTF("init i2c_dev->i2c_comm->base_i2sr [0x%08x]\n", i2c_dev->i2c_comm->base_i2sr);

	//enable i2c module clock
	__REG(i2c_dev->i2c_comm->base_clock) = __REG(i2c_dev->i2c_comm->base_clock) | i2c_dev->i2c_comm->data_clock;

	//i2c module reset
	__REG16(i2c_dev->i2c_comm->base_i2cr) = 0; /* Reset module */

	//i2c module init
	__REG16(i2c_dev->i2c_comm->base_ifdr) = I2C_CLKDIV;	//clock select
	__REG16(i2c_dev->i2c_comm->base_i2sr) = 0;
	__REG16(i2c_dev->i2c_comm->base_i2cr) = I2CR_IEN;	//I2C enable
}

static int i2c_wait_busy(void)
{
	int timeout = I2C_WAIT_TIMEOUT;

	while (!(__REG16(i2c_dev->i2c_comm->base_i2sr) & I2SR_IIF) && --timeout)
		udelay(10);
	__REG16(i2c_dev->i2c_comm->base_i2sr) = 0; /* clear interrupt */

	return timeout;
}

static int i2c_tx_byte(u8 byte)
{
	__REG16(i2c_dev->i2c_comm->base_i2dr) = byte;

	if (!i2c_wait_busy() || __REG16(i2c_dev->i2c_comm->base_i2sr) & I2SR_RX_NO_AK)
	{
		DPRINTF("i2c bus is busy!!!\n");
		return -1;
	}
	DPRINTF("i2c data[%02x] transmitted!!!\n", byte);
	return 0;
}

static u8 i2c_rx_byte(void)
{
	u8 byte;
	if (!i2c_wait_busy())
	{
		DPRINTF("i2c bus is busy!!!\n");
		return -1;
	}
	byte = __REG16(i2c_dev->i2c_comm->base_i2dr);
	DPRINTF("i2c data [%02x] received!!!\n", byte);
	return byte;
}

static int i2c_addr(void)	//uchar chip, uint addr, int alen)
{
	int length = 0;
	__REG16(i2c_dev->i2c_comm->base_i2sr) = 0; /* clear interrupt */
	__REG16(i2c_dev->i2c_comm->base_i2cr) = I2CR_IEN |  I2CR_MSTA | I2CR_MTX;

	if (i2c_tx_byte(i2c_dev->chip << 1))
		return -1;

	length  = i2c_dev->alen;
	while (length--)
		if (i2c_tx_byte((i2c_dev->addr >> (length * 8)) & 0xff))
			return -1;
	return 0;
}

int i2c_read(uchar chip, uint addr, int alen, uchar *buf, int len)
{
	u8 ret;
	int timeout = I2C_READ_TIMEOUT;

	if (i2c_dev->chip == chip)
	{
		if (i2c_dev->alen == alen)
		{
			i2c_dev->addr = addr;
			i2c_dev->dlen = len;
		}
		else
		{
			printf("i2c_dev->alen [%02x],[%02x]\n", i2c_dev->alen, alen);
			return -1;
		}
	}
	else
	{
		printf("i2c_dev->chip [%02x],[%02x]\n", i2c_dev->chip, chip);
		return -1;
	}

	DPRINTF("%s chip: 0x%02x addr: 0x%04x alen: %d len: %d\n",__FUNCTION__, i2c_dev->chip, i2c_dev->addr, i2c_dev->alen, i2c_dev->dlen);

	__REG16(i2c_dev->i2c_comm->base_i2sr) = 0;

	if (i2c_addr()) {
		DPRINTF("i2c_addr failed\n");
		return -1;
	}

	__REG16(i2c_dev->i2c_comm->base_i2cr) = I2CR_IEN |  I2CR_MSTA | I2CR_MTX | I2CR_RSTA;

	if (i2c_tx_byte(i2c_dev->chip << 1 | 1))
		return -1;

	__REG16(i2c_dev->i2c_comm->base_i2cr) = I2CR_IEN |  I2CR_MSTA | ((i2c_dev->dlen == 1) ? I2CR_TX_NO_AK : 0);

	ret = __REG16(i2c_dev->i2c_comm->base_i2dr);

	while (i2c_dev->dlen--) {
		if ((ret = i2c_rx_byte()) < 0)
			return -1;
		*buf++ = ret;
		if (i2c_dev->dlen <= 1)
			__REG16(i2c_dev->i2c_comm->base_i2cr) = I2CR_IEN |  I2CR_MSTA | I2CR_TX_NO_AK;
	}

	i2c_wait_busy();

	__REG16(i2c_dev->i2c_comm->base_i2cr) = I2CR_IEN;

	while (__REG16(i2c_dev->i2c_comm->base_i2sr) & I2SR_IBB && --timeout)
		udelay(1);

	return 0;
}

int i2c_write(uchar chip, uint addr, int alen, uchar *buf, int len)
{
	int timeout = I2C_WRITE_TIMEOUT;

	if (i2c_dev->chip == chip)
	{
		if (i2c_dev->alen == alen)
		{
			i2c_dev->addr = addr;
			i2c_dev->dlen = len;
		}
		else
		{
			printf("i2c_dev->alen [%02x] received!!!\n", alen);
			return -1;
		}
	}
	else
	{
		printf("i2c_dev->chip [%02x] received!!!\n", chip);
		return -1;
	}

	DPRINTF("%s chip: 0x%02x addr: 0x%04x alen: %d len: %d\n",__FUNCTION__, i2c_dev->chip, i2c_dev->addr, i2c_dev->alen, i2c_dev->dlen);

	__REG16(i2c_dev->i2c_comm->base_i2sr) = 0;

	if (i2c_addr()) {
		DPRINTF("i2c_addr failed\n");
		return -1;
	}

	while (i2c_dev->dlen--)
		if (i2c_tx_byte(*buf++))
			return -1;

	__REG16(i2c_dev->i2c_comm->base_i2cr) = I2CR_IEN;

	while (__REG16(i2c_dev->i2c_comm->base_i2sr) & I2SR_IBB && --timeout)
		udelay(1);

	return 0;
}

int i2c_probe(uchar chip)
{
	u8 ret;
	DPRINTF("i2c_probe [0x%02x]\n", chip);

	__REG16(i2c_dev->i2c_comm->base_i2sr) = 0; /* clear interrupt */
	__REG16(i2c_dev->i2c_comm->base_i2cr) = 0; /* Reset module */
	__REG16(i2c_dev->i2c_comm->base_i2cr) = I2CR_IEN;

	__REG16(i2c_dev->i2c_comm->base_i2cr) = I2CR_IEN |  I2CR_MSTA | I2CR_MTX;
	ret = i2c_tx_byte(chip << 1);
	__REG16(i2c_dev->i2c_comm->base_i2cr) = I2CR_IEN | I2CR_MTX;

	return ret;
}

void i2c_dump(int address, int length)
{
	int j;
	u8* buf = (u8*)malloc(sizeof(u8)*length);

	i2c_dev->addr = address;
	i2c_dev->dlen = length;

	DPRINTF("i2c_dev->chip [0x%02x]\n", i2c_dev->chip);
	DPRINTF("i2c_dev->addr [0x%02x]\n", i2c_dev->addr);
	DPRINTF("i2c_dev->dlen [0x%02x]\n", i2c_dev->dlen);

	memset(buf, 0, i2c_dev->dlen);
	i2c_read(i2c_dev->chip, address, 1, buf, length);
	for (j = 0; j < length; j++)
	{
		printf("(device, addr, data)=(0x%02X, 0x%02X, 0x%02X)\n", i2c_dev->chip, address+j, buf[j]);
	}

	free(buf);
}

#endif //CONFIG_MX35_I2C

