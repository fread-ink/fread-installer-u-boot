/*
 * (C) Copyright 2008-2009 Freescale Semiconductor, Inc.
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

#ifndef __IMX_SPI_H__
#define __IMX_SPI_H__

#include <spi.h>

#undef IMX_SPI_DEBUG

#define IMX_SPI_ACTIVE_HIGH     1
#define IMX_SPI_ACTIVE_LOW      0
#define SPI_RETRY_TIMES         1000

#define IMX_SPI_VERSION_0_7	1
#define IMX_SPI_VERSION_2_3	2

struct spi_reg_t {
	u32 ctrl_reg;
	u32 cfg_reg;
};

struct imx_spi_dev_t;

struct imx_spi_funcs_t {
    s32 (*xfer) (struct imx_spi_dev_t *, unsigned int, const void *,
		void *, unsigned long);
    void (*cs_activate) (struct imx_spi_dev_t *);
    void (*cs_deactivate) (struct imx_spi_dev_t *);
};

struct imx_spi_dev_t {
	struct spi_slave slave;
	u32 base;      /* base address of SPI module the device is connected to */
	u32 freq;      /* desired clock freq in Hz for this device */
	u32 ss_pol;    /* ss polarity: 1=active high; 0=active low */
	u32 ss;        /* slave select */
	u32 in_sctl;   /* inactive sclk ctl: 1=stay low; 0=stay high */
	u32 in_dctl;   /* inactive data ctl: 1=stay low; 0=stay high */
	u32 ssctl;     /* single burst mode vs multiple: 0=single; 1=multi */
	u32 sclkpol;   /* sclk polarity: active high=0; active low=1 */
	u32 sclkpha;   /* sclk phase: 0=phase 0; 1=phase1 */
	u32 fifo_sz;   /* fifo size in bytes for either tx or rx. Don't add them up! */
	u32 us_delay;  /* us delay in each xfer */
	u32 version;   /* spi controller version */
	struct imx_spi_funcs_t funcs;     /* spi controller functions */
	struct spi_reg_t reg; /* pointer to a set of SPI registers */
};

extern void board_spi_io_init(struct imx_spi_dev_t *dev);
extern s32 board_spi_get_cfg(struct imx_spi_dev_t *dev);

#if defined(CONFIG_IMX_CSPI)
s32 cspi_init(struct imx_spi_dev_t *dev);
#endif
#if defined(CONFIG_IMX_ECSPI)
s32 ecspi_init(struct imx_spi_dev_t *dev);
#endif
 
static inline struct imx_spi_dev_t *to_imx_spi_slave(struct spi_slave *slave)
{
	return container_of(slave, struct imx_spi_dev_t, slave);
}

#endif /* __IMX_SPI_H__ */
