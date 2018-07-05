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

static s32 ecspi_reset(struct imx_spi_dev_t *dev)
{
	u32 clk_src = mxc_get_clock(MXC_CSPI_CLK);
	s32 pre_div = 0, post_div = 0, i, reg_ctrl, reg_config;
	struct spi_reg_t *reg = &(dev->reg);

	if (dev->freq == 0) {
		printf("Error: desired clock is 0\n");
		return 1;
	}

	reg_ctrl = readl(dev->base + SPI_CON_REG);
	/* Reset spi */
	writel(0, dev->base + SPI_CON_REG);
	writel((reg_ctrl | 0x1), dev->base + SPI_CON_REG);

	/* Control register setup */
	if (clk_src > dev->freq) {
		pre_div = clk_src / dev->freq;
		if (pre_div > 16) {
			post_div = pre_div / 16;
			pre_div = 15;
		}
		if (post_div != 0) {
			for (i = 0; i < 16; i++) {
				if ((1 << i) >= post_div)
					break;
			}
			if (i == 16) {
				printf("Error: no divider can meet the freq: %d\n",
					dev->freq);
				return -1;
			}
			post_div = i;
		}
	}

	debug("pre_div = %d, post_div=%d\n", pre_div, post_div);
	reg_ctrl = (reg_ctrl & ~(3 << 18)) | dev->ss << 18;
	reg_ctrl = (reg_ctrl & ~(0xF << 12)) | pre_div << 12;
	reg_ctrl = (reg_ctrl & ~(0xF << 8)) | post_div << 8;
	reg_ctrl |= 1 << (dev->ss + 4);	/* always set to master mode !!!! */
	reg_ctrl &= ~0x1;		/* disable spi */

	reg_config = readl(dev->base + SPI_CFG_REG);
	/* configuration register setup */
	reg_config = (reg_config & ~(1 << ((dev->ss + 12)))) |
		(dev->ss_pol << (dev->ss + 12));
	reg_config = (reg_config & ~(1 << ((dev->ss + 20)))) |
		(dev->in_sctl << (dev->ss + 20));
	reg_config = (reg_config & ~(1 << ((dev->ss + 16)))) |
		(dev->in_dctl << (dev->ss + 16));
	reg_config = (reg_config & ~(1 << ((dev->ss + 8)))) |
		(dev->ssctl << (dev->ss + 8));
	reg_config = (reg_config & ~(1 << ((dev->ss + 4)))) |
		(dev->sclkpol << (dev->ss + 4));
	reg_config = (reg_config & ~(1 << ((dev->ss + 0)))) |
		(dev->sclkpha << (dev->ss + 0));

	debug("reg_ctrl = 0x%x\n", reg_ctrl);
	writel(reg_ctrl, dev->base + SPI_CON_REG);
	debug("reg_config = 0x%x\n", reg_config);
	writel(reg_config, dev->base + SPI_CFG_REG);

	/* save config register and control register */
	reg->cfg_reg  = reg_config;
	reg->ctrl_reg = reg_ctrl;

	/* clear interrupt reg */
	writel(0, dev->base + SPI_INT_REG);
	writel(3 << 6, dev->base + SPI_STAT_REG);

	return 0;
}

void ecspi_cs_activate(struct imx_spi_dev_t *dev)
{
    board_spi_io_init(dev);
}

void ecspi_cs_deactivate(struct imx_spi_dev_t *dev)
{
    writel(0, dev->base + SPI_CON_REG);
}

/*
 * SPI transfer:
 *
 * See include/spi.h and http://www.altera.com/literature/ds/ds_nios_spi.pdf
 * for more informations.
 */
int ecspi_xfer(struct imx_spi_dev_t *dev, unsigned int bitlen, const void *dout,
		void *din, unsigned long flags)
{
	s32 val = SPI_RETRY_TIMES;
	u32 *p_buf;
	u32 reg;
	s32 len = 0,
		ret_val = 0;
	s32 burst_bytes = bitlen >> 3;
	s32 tmp = 0;
	struct spi_reg_t *spi_reg = &(dev->reg);

	if (!dev)
		return -1;

	if (burst_bytes > (MAX_SPI_BYTES)) {
		printf("Error: maximum burst size is 0x%x bytes, asking 0x%x\n",
				MAX_SPI_BYTES, burst_bytes);
		return -1;
	}

	if (flags & SPI_XFER_BEGIN) {
		ecspi_cs_activate(dev);

		if (spi_reg->ctrl_reg == 0) {
			printf("Error: spi(base=0x%x) has not been initialized yet\n",
					dev->base);
			return -1;
		}
		spi_reg->ctrl_reg = (spi_reg->ctrl_reg & ~0xFFF00000) | \
					((burst_bytes * 8 - 1) << 20);

		writel(spi_reg->ctrl_reg | 0x1, dev->base + SPI_CON_REG);
		writel(spi_reg->cfg_reg, dev->base + SPI_CFG_REG);
		debug("ctrl_reg=0x%x, cfg_reg=0x%x\n",
					 readl(dev->base + SPI_CON_REG),
					 readl(dev->base + SPI_CFG_REG));

		/* move data to the tx fifo */
		if (dout) {
			for (p_buf = (u32 *)dout, len = burst_bytes; len > 0;
				p_buf++, len -= 4)
				writel(*p_buf, dev->base + SPI_TX_DATA);
		} else {
			for (len = burst_bytes; len > 0; len -= 4)
				writel(tmp, dev->base + SPI_TX_DATA);
		}

		reg = readl(dev->base + SPI_CON_REG);
		reg |= (1 << 2); /* set xch bit */
		debug("control reg = 0x%08x\n", reg);
		writel(reg, dev->base + SPI_CON_REG);

		/* poll on the TC bit (transfer complete) */
		while ((val-- > 0) &&
			(readl(dev->base + SPI_STAT_REG) & (1 << 7)) == 0) {
			udelay(100);
		}

		/* clear the TC bit */
		writel(3 << 6, dev->base + SPI_STAT_REG);
		if (val <= 0) {
			printf("Error: re-tried %d times without response. Give up\n",
					SPI_RETRY_TIMES);
			ret_val = -1;
			goto error;
		}
	}

	/* move data in the rx buf */
	if (flags & SPI_XFER_END) {
		if (din) {
			for (p_buf = (u32 *)din, len = burst_bytes; len > 0;
				++p_buf, len -= 4)
				*p_buf = readl(dev->base + SPI_RX_DATA);
		} else {
			for (len = burst_bytes; len > 0; len -= 4)
				tmp = readl(dev->base + SPI_RX_DATA);
		}

		ecspi_cs_deactivate(dev);
	}

	return ret_val;

error:
	ecspi_cs_deactivate(dev);
	return ret_val;
}

s32 ecspi_init(struct imx_spi_dev_t *dev)
{
    dev->funcs.xfer = ecspi_xfer;
    dev->funcs.cs_activate = ecspi_cs_activate;
    dev->funcs.cs_deactivate = ecspi_cs_deactivate;

    return ecspi_reset(dev);
}
