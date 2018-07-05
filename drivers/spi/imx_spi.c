/*
 * (C) Copyright 2008-2010 Freescale Semiconductor, Inc.
 *
 * See file CREDITS for list of people who contributed to this
 * project.
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; either version 2 of
 * the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston,
 * MA 02111-1307 USA
 */

#include <config.h>
#include <common.h>
#include <spi.h>
#include <asm/errno.h>
#include <linux/types.h>
#include <asm/io.h>
#include <malloc.h>

#include <imx_spi.h>

#define	SPI_RX_DATA		0x0
#define SPI_TX_DATA		0x4
#define SPI_CON_REG		0x8
#define SPI_CFG_REG		0xC
#define SPI_INT_REG		0x10
#define SPI_DMA_REG		0x14
#define SPI_STAT_REG		0x18
#define SPI_PERIOD_REG		0x1C


void spi_init(void)
{
}

struct spi_slave *spi_setup_slave(unsigned int bus, unsigned int cs,
		unsigned int max_hz, unsigned int mode)
{
    struct imx_spi_dev_t *imx_spi_slave = NULL;
    s32 ret = -1;

    if (!spi_cs_is_valid(bus, cs))
	return NULL;
    
    imx_spi_slave = (struct imx_spi_dev_t *)malloc(sizeof(struct imx_spi_dev_t));
    if (!imx_spi_slave)
	return NULL;

    imx_spi_slave->slave.bus = bus;
    imx_spi_slave->slave.cs = cs;
    
    if (board_spi_get_cfg(imx_spi_slave)) {
	printf("Error: invalid config!\n");
	return NULL;
    }

    board_spi_io_init(imx_spi_slave);

    if (imx_spi_slave->version == IMX_SPI_VERSION_0_7) {
#if defined(CONFIG_IMX_CSPI)
	ret = cspi_init(imx_spi_slave);
#endif
    } else if (imx_spi_slave->version == IMX_SPI_VERSION_2_3) {
#if defined(CONFIG_IMX_ECSPI)
	ret = ecspi_init(imx_spi_slave);
#endif
    } 

    if (ret) {
	printf("Error: Invalid version\n");
	return NULL;
    }

    return &(imx_spi_slave->slave);
}

void spi_free_slave(struct spi_slave *slave)
{
    struct imx_spi_dev_t *imx_spi_slave;

    if (slave) {
	imx_spi_slave = to_imx_spi_slave(slave);
	free(imx_spi_slave);
    }
}

int spi_claim_bus(struct spi_slave *slave)
{
    return 0;
}

void spi_release_bus(struct spi_slave *slave)
{

}

/*
 * SPI transfer:
 *
 * See include/spi.h and http://www.altera.com/literature/ds/ds_nios_spi.pdf
 * for more informations.
 */
int spi_xfer(struct spi_slave *slave, unsigned int bitlen, const void *dout,
		void *din, unsigned long flags)
{
    struct imx_spi_dev_t *dev = to_imx_spi_slave(slave);
    
    return dev->funcs.xfer(dev, bitlen, dout, din, flags);
}

int spi_cs_is_valid(unsigned int bus, unsigned int cs)
{
    return 1;
}

void spi_cs_activate(struct spi_slave *slave)
{
    struct imx_spi_dev_t *dev = to_imx_spi_slave(slave);

    dev->funcs.cs_activate(dev);
}

void spi_cs_deactivate(struct spi_slave *slave)
{
    struct imx_spi_dev_t *dev = to_imx_spi_slave(slave);
    
    dev->funcs.cs_deactivate(dev);
}

