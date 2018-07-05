/*
 * Copyright (C) 2007, Guennadi Liakhovetski <lg@denx.de>
 *
 * (C) Copyright 2009 Freescale Semiconductor, Inc.
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
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston,
 * MA 02111-1307 USA
 */

#include <common.h>
#include <asm/io.h>
#include <asm/arch/mx51.h>
#include <asm/arch/mx51_pins.h>
#include <asm/arch/iomux.h>
#include <asm/errno.h>
#include <i2c.h>
#include <linux/ctype.h>
#include "board-imx51_banjo.h"
#include <imx_spi.h>
#include <pmic_13892.h>
#include <usb/file_storage.h>
#if defined (CONFIG_CMD_IDME)
#include <idme.h>
#endif

#include <asm/errno.h>

#ifdef CONFIG_MXC_KPD
#include <asm/arch/keypad.h>
#endif

#ifdef CONFIG_HW_WATCHDOG
#include <watchdog.h>
#endif

/* watchdog registers */
#define WDOG_WCR __REG16(WDOG1_BASE_ADDR + 0x00)
#define WDOG_WSR __REG16(WDOG1_BASE_ADDR + 0x02)
#define WDOG_WRSR __REG16(WDOG1_BASE_ADDR + 0x04)
#define WDOG_WICR __REG16(WDOG1_BASE_ADDR + 0x06)
#define WDOG_WMCR __REG16(WDOG1_BASE_ADDR + 0x08)

#ifdef CONFIG_MMC
#include <mmc.h>
#include <fsl_esdhc.h>
#endif

#ifdef CONFIG_ARCH_MMU
#include <asm/mmu.h>
#include <asm/arch/mmu.h>
#endif

DECLARE_GLOBAL_DATA_PTR;

static u32 system_rev;
static enum boot_device boot_dev;
u32	mx51_io_base_addr;

/* board id and serial number. */
static u8 serial_number[16+1];
static u8 board_id[16+1];

static inline void setup_boot_device(void)
{
	uint *fis_addr = (uint *)IRAM_BASE_ADDR;

	switch (*fis_addr) {
	case NAND_FLASH_BOOT:
		boot_dev = NAND_BOOT;
		break;
	case SPI_NOR_FLASH_BOOT:
		boot_dev = SPI_NOR_BOOT;
		break;
	case MMC_FLASH_BOOT:
		boot_dev = MMC_BOOT;
		break;
	default:
		{
			uint soc_sbmr = readl(SRC_BASE_ADDR + 0x4);
			uint bt_mem_ctl = soc_sbmr & 0x00000003;
			uint bt_mem_type = (soc_sbmr & 0x00000180) >> 7;

			switch (bt_mem_ctl) {
			case 0x3:
				if (bt_mem_type == 0)
					boot_dev = MMC_BOOT;
				else if (bt_mem_type == 3)
					boot_dev = SPI_NOR_BOOT;
				else
					boot_dev = UNKNOWN_BOOT;
				break;
			case 0x1:
				boot_dev = NAND_BOOT;
				break;
			default:
				boot_dev = UNKNOWN_BOOT;
			}
		}
		break;
	}
}

enum boot_device get_boot_device(void)
{
	return boot_dev;
}

u32 get_board_rev(void)
{
	return system_rev;
}

static inline void setup_soc_rev(void)
{
	int reg;
	reg = __REG(ROM_SI_REV);
	switch (reg) {
	case 0x02:
		system_rev = 0x51000 | CHIP_REV_1_1;
		break;
	case 0x10:
		if ((__REG(GPIO1_BASE_ADDR + 0x0) & (0x1 << 22)) == 0) {
			system_rev = 0x51000 | CHIP_REV_2_5;
		} else {
			system_rev = 0x51000 | CHIP_REV_2_0;
		}
		break;
	case 0x20:
		system_rev = 0x51000 | CHIP_REV_3_0;
		break;
	default:
		system_rev = 0x51000 | CHIP_REV_1_0;
	}
}

static inline void set_board_rev(int rev)
{
	system_rev |= (rev & 0xF) << 8;
}

inline int is_soc_rev(int rev)
{
	return (system_rev & 0xFF) - rev;
}

int setup_board_info(void)
{
#if defined(CONFIG_CMD_IDME)
    if (idme_get_var("boardid", (char *) board_id, 16 + 1)) 
#endif
    {
	/* not found: clean up garbage characters. */
	memset(board_id, 0, 16 + 1);
    }

#if defined(CONFIG_CMD_IDME)
    if (idme_get_var("serial", (char *) serial_number, 16 + 1)) 
#endif
    {
	/* not found: clean up garbage characters. */
	memset(serial_number, 0, 16 + 1);
    }

    return 0;
}

int board_info_valid (u8 *info)
{
    int i;

    for (i = 0; i < 16; i++) {
	if (!isalnum(info[i]))
	    return 0;
    }

    return 1;
}

/*************************************************************************
 * get_board_serial16() - setup to pass kernel serial number information
 *      16-byte alphanumeric containing the serial number.
 *************************************************************************/
const u8 *get_board_serial16(void)
{
    if (!board_info_valid(serial_number))
	return (u8 *) "0000000000000000";
    else
	return serial_number;
}

/*************************************************************************
 * get_board_id16() - setup to pass kernel board revision information
 *      16-byte alphanumeric containing the board revision.
 *************************************************************************/
const u8 *get_board_id16(void)
{
    if (!board_info_valid(board_id))
	return (u8 *) "0000000000000000";
    else
	return board_id;
}


#ifdef CONFIG_ARCH_MMU
void board_mmu_init(void)
{
	unsigned long ttb_base = PHYS_SDRAM_1 + 0x4000;
	unsigned long i;

	/*
	* Set the TTB register
	*/
	asm volatile ("mcr  p15,0,%0,c2,c0,0" : : "r"(ttb_base) /*:*/);

	/*
	* Set the Domain Access Control Register
	*/
	i = ARM_ACCESS_DACR_DEFAULT;
	asm volatile ("mcr  p15,0,%0,c3,c0,0" : : "r"(i) /*:*/);

	/*
	* First clear all TT entries - ie Set them to Faulting
	*/
	memset((void *)ttb_base, 0, ARM_FIRST_LEVEL_PAGE_TABLE_SIZE);
	/* Actual   Virtual  Size   Attributes          Function */
	/* Base     Base     MB     cached? buffered?  access permissions */
	/* xxx00000 xxx00000 */
	X_ARM_MMU_SECTION(0x000, 0x200, 0x1,
			ARM_UNCACHEABLE, ARM_UNBUFFERABLE,
			ARM_ACCESS_PERM_RW_RW); /* ROM */
	X_ARM_MMU_SECTION(0x1FF, 0x1FF, 0x001,
			ARM_UNCACHEABLE, ARM_UNBUFFERABLE,
			ARM_ACCESS_PERM_RW_RW); /* IRAM */
	X_ARM_MMU_SECTION(0x300, 0x300, 0x100,
			ARM_UNCACHEABLE, ARM_UNBUFFERABLE,
			ARM_ACCESS_PERM_RW_RW); /* GPU */
	X_ARM_MMU_SECTION(0x400, 0x400, 0x200,
			ARM_UNCACHEABLE, ARM_UNBUFFERABLE,
			ARM_ACCESS_PERM_RW_RW); /* IPUv3D */
	X_ARM_MMU_SECTION(0x600, 0x600, 0x300,
			ARM_UNCACHEABLE, ARM_UNBUFFERABLE,
			ARM_ACCESS_PERM_RW_RW); /* periperals */
	X_ARM_MMU_SECTION(0x900, 0x000, 0x1FF,
			ARM_CACHEABLE, ARM_BUFFERABLE,
			ARM_ACCESS_PERM_RW_RW); /* SDRAM */
	X_ARM_MMU_SECTION(0x900, 0x900, 0x200,
			ARM_CACHEABLE, ARM_BUFFERABLE,
			ARM_ACCESS_PERM_RW_RW); /* SDRAM */
	X_ARM_MMU_SECTION(0x900, 0xE00, 0x200,
			ARM_UNCACHEABLE, ARM_UNBUFFERABLE,
			ARM_ACCESS_PERM_RW_RW); /* SDRAM 0:128M*/
	X_ARM_MMU_SECTION(0xB80, 0xB80, 0x10,
			ARM_UNCACHEABLE, ARM_UNBUFFERABLE,
			ARM_ACCESS_PERM_RW_RW); /* CS1 EIM control*/
	X_ARM_MMU_SECTION(0xCC0, 0xCC0, 0x040,
			ARM_UNCACHEABLE, ARM_UNBUFFERABLE,
			ARM_ACCESS_PERM_RW_RW); /* CS4/5/NAND Flash buffer */
}
#endif

int dram_init(void)
{
	gd->bd->bi_dram[0].start = PHYS_SDRAM_1;
	gd->bd->bi_dram[0].size = PHYS_SDRAM_1_SIZE;
	return 0;
}

static void setup_uart(void)
{
	unsigned int pad = PAD_CTL_HYS_ENABLE | PAD_CTL_PKE_ENABLE |
			 PAD_CTL_PUE_PULL | PAD_CTL_DRV_HIGH;
	mxc_request_iomux(MX51_PIN_UART1_RXD, IOMUX_CONFIG_ALT0);
	mxc_iomux_set_pad(MX51_PIN_UART1_RXD, pad | PAD_CTL_SRE_FAST);
	mxc_request_iomux(MX51_PIN_UART1_TXD, IOMUX_CONFIG_ALT0);
	mxc_iomux_set_pad(MX51_PIN_UART1_TXD, pad | PAD_CTL_SRE_FAST);
	mxc_request_iomux(MX51_PIN_UART1_RTS, IOMUX_CONFIG_ALT0);
	mxc_iomux_set_pad(MX51_PIN_UART1_RTS, pad);
	mxc_request_iomux(MX51_PIN_UART1_CTS, IOMUX_CONFIG_ALT0);
	mxc_iomux_set_pad(MX51_PIN_UART1_CTS, pad);
	/* enable GPIO1_9 for CLK0 and GPIO1_8 for CLK02 */
	writel(0x00000004, 0x73fa83e8);
	writel(0x00000004, 0x73fa83ec);
}

#ifdef CONFIG_MXC_NAND
void setup_nfc(void)
{
	/* Enable NFC IOMUX */
	mxc_request_iomux(MX51_PIN_NANDF_CS0, IOMUX_CONFIG_ALT0);
	mxc_request_iomux(MX51_PIN_NANDF_CS1, IOMUX_CONFIG_ALT0);
	mxc_request_iomux(MX51_PIN_NANDF_CS2, IOMUX_CONFIG_ALT0);
	mxc_request_iomux(MX51_PIN_NANDF_CS3, IOMUX_CONFIG_ALT0);
	mxc_request_iomux(MX51_PIN_NANDF_CS4, IOMUX_CONFIG_ALT0);
	mxc_request_iomux(MX51_PIN_NANDF_CS5, IOMUX_CONFIG_ALT0);
	mxc_request_iomux(MX51_PIN_NANDF_CS6, IOMUX_CONFIG_ALT0);
	mxc_request_iomux(MX51_PIN_NANDF_CS7, IOMUX_CONFIG_ALT0);
}
#endif

#ifdef CONFIG_MXC_FEC
static void setup_expio(void)
{
	u32 reg;
	/* CS5 setup */
	mxc_request_iomux(MX51_PIN_EIM_CS5, IOMUX_CONFIG_ALT0);
	writel(0x00410089, WEIM_BASE_ADDR + 0x78 + CSGCR1);
	writel(0x00000002, WEIM_BASE_ADDR + 0x78 + CSGCR2);
	/* RWSC=50, RADVA=2, RADVN=6, OEA=0, OEN=0, RCSA=0, RCSN=0 */
	writel(0x32260000, WEIM_BASE_ADDR + 0x78 + CSRCR1);
	/* APR = 0 */
	writel(0x00000000, WEIM_BASE_ADDR + 0x78 + CSRCR2);
	/* WAL=0, WBED=1, WWSC=50, WADVA=2, WADVN=6, WEA=0, WEN=0,
	 * WCSA=0, WCSN=0
	 */
	writel(0x72080F00, WEIM_BASE_ADDR + 0x78 + CSWCR1);
	if ((readw(CS5_BASE_ADDR + PBC_ID_AAAA) == 0xAAAA) &&
	    (readw(CS5_BASE_ADDR + PBC_ID_5555) == 0x5555)) {
		if (is_soc_rev(CHIP_REV_2_0) < 0) {
			reg = readl(CCM_BASE_ADDR + CLKCTL_CBCDR);
			reg = (reg & (~0x70000)) | 0x30000;
			writel(reg, CCM_BASE_ADDR + CLKCTL_CBCDR);
			/* make sure divider effective */
			while (readl(CCM_BASE_ADDR + CLKCTL_CDHIPR) != 0)
				;
			writel(0x0, CCM_BASE_ADDR + CLKCTL_CCDR);
		}
		mx51_io_base_addr = CS5_BASE_ADDR;
	} else {
		/* CS1 */
		writel(0x00410089, WEIM_BASE_ADDR + 0x18 + CSGCR1);
		writel(0x00000002, WEIM_BASE_ADDR + 0x18 + CSGCR2);
		/*  RWSC=50, RADVA=2, RADVN=6, OEA=0, OEN=0, RCSA=0, RCSN=0 */
		writel(0x32260000, WEIM_BASE_ADDR + 0x18 + CSRCR1);
		/* APR=0 */
		writel(0x00000000, WEIM_BASE_ADDR + 0x18 + CSRCR2);
		/* WAL=0, WBED=1, WWSC=50, WADVA=2, WADVN=6, WEA=0,
		 * WEN=0, WCSA=0, WCSN=0
		 */
		writel(0x72080F00, WEIM_BASE_ADDR + 0x18 + CSWCR1);
		mx51_io_base_addr = CS1_BASE_ADDR;
	}

	/* Reset interrupt status reg */
	writew(0x1F, mx51_io_base_addr + PBC_INT_REST);
	writew(0x00, mx51_io_base_addr + PBC_INT_REST);
	writew(0xFFFF, mx51_io_base_addr + PBC_INT_MASK);

	/* Reset the XUART and Ethernet controllers */
	reg = readw(mx51_io_base_addr + PBC_SW_RESET);
	reg |= 0x9;
	writew(reg, mx51_io_base_addr + PBC_SW_RESET);
	reg &= ~0x9;
	writew(reg, mx51_io_base_addr + PBC_SW_RESET);
}
#endif

s32 board_spi_get_cfg(struct imx_spi_dev_t *dev)
{
    switch (dev->slave.bus) {
#if defined(CONFIG_IMX_ECSPI)
       case 1:
	dev->base = CSPI1_BASE_ADDR;
	dev->freq = 1000000;
	dev->ss = 0;
	dev->ss_pol = IMX_SPI_ACTIVE_HIGH;
	dev->fifo_sz = 32;
	dev->us_delay = 0;
	dev->version = IMX_SPI_VERSION_2_3;
	break;
#endif

     default:
       printf("Invalid SPI Bus ID %d! \n", dev->slave.bus);
       return 1;
    }

    return 0;
}

void board_spi_io_init(struct imx_spi_dev_t *dev)
{
	switch (dev->base) {
	case CSPI1_BASE_ADDR:
		/* 000: Select mux mode: ALT0 mux port: MOSI of instance: ecspi1 */
		mxc_request_iomux(MX51_PIN_CSPI1_MOSI, IOMUX_CONFIG_ALT0);
		mxc_iomux_set_pad(MX51_PIN_CSPI1_MOSI, 0x105);

		/* 000: Select mux mode: ALT0 mux port: MISO of instance: ecspi1. */
		mxc_request_iomux(MX51_PIN_CSPI1_MISO, IOMUX_CONFIG_ALT0);
		mxc_iomux_set_pad(MX51_PIN_CSPI1_MISO, 0x105);

		if (dev->ss == 0) {
			/* de-select SS1 of instance: ecspi1. */
			mxc_request_iomux(MX51_PIN_CSPI1_SS1, IOMUX_CONFIG_ALT3);
			mxc_iomux_set_pad(MX51_PIN_CSPI1_SS1, 0x85);
			/* 000: Select mux mode: ALT0 mux port: SS0 of instance: ecspi1. */
			mxc_request_iomux(MX51_PIN_CSPI1_SS0, IOMUX_CONFIG_ALT0);
			mxc_iomux_set_pad(MX51_PIN_CSPI1_SS0, 0x185);
		} else if (dev->ss == 1) {
			/* de-select SS0 of instance: ecspi1. */
			mxc_request_iomux(MX51_PIN_CSPI1_SS0, IOMUX_CONFIG_ALT3);
			mxc_iomux_set_pad(MX51_PIN_CSPI1_SS0, 0x85);
			/* 000: Select mux mode: ALT0 mux port: SS1 of instance: ecspi1. */
			mxc_request_iomux(MX51_PIN_CSPI1_SS1, IOMUX_CONFIG_ALT0);
			mxc_iomux_set_pad(MX51_PIN_CSPI1_SS1, 0x105);
		}

		/* 000: Select mux mode: ALT0 mux port: RDY of instance: ecspi1. */
		mxc_request_iomux(MX51_PIN_CSPI1_RDY, IOMUX_CONFIG_ALT0);
		mxc_iomux_set_pad(MX51_PIN_CSPI1_RDY, 0x180);

		/* 000: Select mux mode: ALT0 mux port: SCLK of instance: ecspi1. */
		mxc_request_iomux(MX51_PIN_CSPI1_SCLK, IOMUX_CONFIG_ALT0);
		mxc_iomux_set_pad(MX51_PIN_CSPI1_SCLK, 0x105);
		break;
	case CSPI2_BASE_ADDR:
	default:
		break;
	}
}

#ifdef CONFIG_MXC_FEC
static void setup_fec(void)
{
	/*FEC_MDIO*/
	writel(0x3, IOMUXC_BASE_ADDR + 0x0D4);
	writel(0x1FD, IOMUXC_BASE_ADDR + 0x0468);
	writel(0x0, IOMUXC_BASE_ADDR + 0x0954);

	/*FEC_MDC*/
	writel(0x2, IOMUXC_BASE_ADDR + 0x13C);
	writel(0x2004, IOMUXC_BASE_ADDR + 0x0524);

	/* FEC RDATA[3] */
	writel(0x3, IOMUXC_BASE_ADDR + 0x0EC);
	writel(0x180, IOMUXC_BASE_ADDR + 0x0480);
	writel(0x0, IOMUXC_BASE_ADDR + 0x0964);

	/* FEC RDATA[2] */
	writel(0x3, IOMUXC_BASE_ADDR + 0x0E8);
	writel(0x180, IOMUXC_BASE_ADDR + 0x047C);
	writel(0x0, IOMUXC_BASE_ADDR + 0x0960);

	/* FEC RDATA[1] */
	writel(0x3, IOMUXC_BASE_ADDR + 0x0d8);
	writel(0x180, IOMUXC_BASE_ADDR + 0x046C);
	writel(0x0, IOMUXC_BASE_ADDR + 0x095C);

	/* FEC RDATA[0] */
	writel(0x2, IOMUXC_BASE_ADDR + 0x016C);
	writel(0x2180, IOMUXC_BASE_ADDR + 0x0554);
	writel(0x0, IOMUXC_BASE_ADDR + 0x0958);

	/* FEC TDATA[3] */
	writel(0x2, IOMUXC_BASE_ADDR + 0x148);
	writel(0x2004, IOMUXC_BASE_ADDR + 0x0530);

	/* FEC TDATA[2] */
	writel(0x2, IOMUXC_BASE_ADDR + 0x144);
	writel(0x2004, IOMUXC_BASE_ADDR + 0x052C);

	/* FEC TDATA[1] */
	writel(0x2, IOMUXC_BASE_ADDR + 0x140);
	writel(0x2004, IOMUXC_BASE_ADDR + 0x0528);

	/* FEC TDATA[0] */
	writel(0x2, IOMUXC_BASE_ADDR + 0x0170);
	writel(0x2004, IOMUXC_BASE_ADDR + 0x0558);

	/* FEC TX_EN */
	writel(0x1, IOMUXC_BASE_ADDR + 0x014C);
	writel(0x2004, IOMUXC_BASE_ADDR + 0x0534);

	/* FEC TX_ER */
	writel(0x2, IOMUXC_BASE_ADDR + 0x0138);
	writel(0x2004, IOMUXC_BASE_ADDR + 0x0520);

	/* FEC TX_CLK */
	writel(0x1, IOMUXC_BASE_ADDR + 0x0150);
	writel(0x2180, IOMUXC_BASE_ADDR + 0x0538);
	writel(0x0, IOMUXC_BASE_ADDR + 0x0974);

	/* FEC COL */
	writel(0x1, IOMUXC_BASE_ADDR + 0x0124);
	writel(0x2180, IOMUXC_BASE_ADDR + 0x0500);
	writel(0x0, IOMUXC_BASE_ADDR + 0x094c);

	/* FEC RX_CLK */
	writel(0x1, IOMUXC_BASE_ADDR + 0x0128);
	writel(0x2180, IOMUXC_BASE_ADDR + 0x0504);
	writel(0x0, IOMUXC_BASE_ADDR + 0x0968);

	/* FEC CRS */
	writel(0x3, IOMUXC_BASE_ADDR + 0x0f4);
	writel(0x180, IOMUXC_BASE_ADDR + 0x0488);
	writel(0x0, IOMUXC_BASE_ADDR + 0x0950);

	/* FEC RX_ER */
	writel(0x3, IOMUXC_BASE_ADDR + 0x0f0);
	writel(0x180, IOMUXC_BASE_ADDR + 0x0484);
	writel(0x0, IOMUXC_BASE_ADDR + 0x0970);

	/* FEC RX_DV */
	writel(0x2, IOMUXC_BASE_ADDR + 0x164);
	writel(0x2180, IOMUXC_BASE_ADDR + 0x054C);
	writel(0x0, IOMUXC_BASE_ADDR + 0x096C);
}
#endif

#ifdef CONFIG_NET_MULTI

int board_eth_init(bd_t *bis)
{
	int rc = -ENODEV;

	return rc;
}
#endif

#ifdef CONFIG_MMC

struct fsl_esdhc_cfg mmc_cfg;

int esdhc_gpio_init(int if_num)
{
	u32 interface_esdhc = 0;
	s32 status = 0;
	u32 pad = 0;

	interface_esdhc = if_num;

	switch (interface_esdhc) {
	case 0:
		mmc_cfg.esdhc_base = (u32 *)MMC_SDHC1_BASE_ADDR;

		/* BEN - use default pad values */
		mxc_request_iomux(MX51_PIN_SD1_CMD,
			  IOMUX_CONFIG_ALT0 | IOMUX_CONFIG_SION);
		mxc_request_iomux(MX51_PIN_SD1_CLK,
			  IOMUX_CONFIG_ALT0 | IOMUX_CONFIG_SION);

		mxc_request_iomux(MX51_PIN_SD1_DATA0,
				IOMUX_CONFIG_ALT0 | IOMUX_CONFIG_SION);
		mxc_request_iomux(MX51_PIN_SD1_DATA1,
				IOMUX_CONFIG_ALT0 | IOMUX_CONFIG_SION);
		mxc_request_iomux(MX51_PIN_SD1_DATA2,
				IOMUX_CONFIG_ALT0 | IOMUX_CONFIG_SION);
		mxc_request_iomux(MX51_PIN_SD1_DATA3,
				IOMUX_CONFIG_ALT0 | IOMUX_CONFIG_SION);

		/* DATA4-7 for 8-bit mode */
		mxc_request_iomux(MX51_PIN_SD2_DATA0, IOMUX_CONFIG_ALT1);
		mxc_request_iomux(MX51_PIN_SD2_DATA1, IOMUX_CONFIG_ALT1);
		mxc_request_iomux(MX51_PIN_SD2_DATA2, IOMUX_CONFIG_ALT1);
		mxc_request_iomux(MX51_PIN_SD2_DATA3, IOMUX_CONFIG_ALT1);

		break;
	case 1:
		mmc_cfg.esdhc_base = (u32 *)MMC_SDHC2_BASE_ADDR;

		pad = PAD_CTL_DRV_MAX | PAD_CTL_47K_PU | PAD_CTL_SRE_FAST;

		mxc_request_iomux(MX51_PIN_SD2_CMD,
			  IOMUX_CONFIG_ALT0 | IOMUX_CONFIG_SION);
		mxc_request_iomux(MX51_PIN_SD2_CLK,
			  IOMUX_CONFIG_ALT0 | IOMUX_CONFIG_SION);

		mxc_request_iomux(MX51_PIN_SD2_DATA0,
				IOMUX_CONFIG_ALT0 | IOMUX_CONFIG_SION);
		mxc_request_iomux(MX51_PIN_SD2_DATA1,
				IOMUX_CONFIG_ALT0 | IOMUX_CONFIG_SION);
		mxc_request_iomux(MX51_PIN_SD2_DATA2,
				IOMUX_CONFIG_ALT0 | IOMUX_CONFIG_SION);
		mxc_request_iomux(MX51_PIN_SD2_DATA3,
				IOMUX_CONFIG_ALT0 | IOMUX_CONFIG_SION);
		mxc_iomux_set_pad(MX51_PIN_SD2_CMD, pad);
		mxc_iomux_set_pad(MX51_PIN_SD2_CLK, pad);
		mxc_iomux_set_pad(MX51_PIN_SD2_DATA0, pad);
		mxc_iomux_set_pad(MX51_PIN_SD2_DATA1, pad);
		mxc_iomux_set_pad(MX51_PIN_SD2_DATA2, pad);
		mxc_iomux_set_pad(MX51_PIN_SD2_DATA3, pad);
		break;
	case 2:
		mmc_cfg.esdhc_base = (u32 *)MMC_SDHC3_BASE_ADDR;

		pad = PAD_CTL_DRV_MAX | PAD_CTL_47K_PU | PAD_CTL_HYS_ENABLE | 
		    PAD_CTL_PKE_ENABLE | PAD_CTL_SRE_FAST;

		mxc_request_iomux(MX51_PIN_NANDF_RDY_INT,
			  IOMUX_CONFIG_ALT5 | IOMUX_CONFIG_SION);
		mxc_request_iomux(MX51_PIN_NANDF_CS7,
			  IOMUX_CONFIG_ALT5 | IOMUX_CONFIG_SION);

		mxc_request_iomux(MX51_PIN_NANDF_WE_B,
				IOMUX_CONFIG_ALT2 | IOMUX_CONFIG_SION);
		mxc_request_iomux(MX51_PIN_NANDF_RE_B,
				IOMUX_CONFIG_ALT2 | IOMUX_CONFIG_SION);
		mxc_request_iomux(MX51_PIN_NANDF_WP_B,
				IOMUX_CONFIG_ALT2 | IOMUX_CONFIG_SION);
		mxc_request_iomux(MX51_PIN_NANDF_RB0,
				IOMUX_CONFIG_ALT2 | IOMUX_CONFIG_SION);
		mxc_iomux_set_pad(MX51_PIN_NANDF_RDY_INT, pad);
		mxc_iomux_set_pad(MX51_PIN_NANDF_CS7, pad);
		mxc_iomux_set_pad(MX51_PIN_NANDF_WE_B, pad);
		mxc_iomux_set_pad(MX51_PIN_NANDF_RE_B, pad);
		mxc_iomux_set_pad(MX51_PIN_NANDF_WP_B, pad);
		mxc_iomux_set_pad(MX51_PIN_NANDF_RB0, pad);
		break;
	  case 3:
	        mmc_cfg.esdhc_base = (u32 *)MMC_SDHC4_BASE_ADDR;

		mxc_request_iomux(MX51_PIN_NANDF_RB1,
			  IOMUX_CONFIG_ALT5 | IOMUX_CONFIG_SION);
		mxc_request_iomux(MX51_PIN_NANDF_CS2,
			  IOMUX_CONFIG_ALT5 | IOMUX_CONFIG_SION);

		mxc_request_iomux(MX51_PIN_NANDF_CS3,
				IOMUX_CONFIG_ALT5 | IOMUX_CONFIG_SION);
		mxc_request_iomux(MX51_PIN_NANDF_CS4,
				IOMUX_CONFIG_ALT5 | IOMUX_CONFIG_SION);
		mxc_request_iomux(MX51_PIN_NANDF_CS5,
				IOMUX_CONFIG_ALT5 | IOMUX_CONFIG_SION);
		mxc_request_iomux(MX51_PIN_NANDF_CS6,
				IOMUX_CONFIG_ALT5 | IOMUX_CONFIG_SION);

		pad = PAD_CTL_DRV_LOW | PAD_CTL_DRV_VOT_HIGH | PAD_CTL_ODE_OPENDRAIN_NONE |
				PAD_CTL_HYS_NONE | PAD_CTL_100K_PU | PAD_CTL_PUE_PULL |
				PAD_CTL_PKE_ENABLE;

		mxc_iomux_set_pad(MX51_PIN_NANDF_RB1, pad);

		pad = PAD_CTL_DRV_HIGH | PAD_CTL_DRV_VOT_HIGH | PAD_CTL_ODE_OPENDRAIN_NONE |
				PAD_CTL_HYS_NONE | PAD_CTL_PUE_PULL | PAD_CTL_PKE_ENABLE;

		mxc_iomux_set_pad(MX51_PIN_NANDF_CS2, pad | PAD_CTL_100K_PU);
		mxc_iomux_set_pad(MX51_PIN_NANDF_CS3, pad | PAD_CTL_100K_PU);
		mxc_iomux_set_pad(MX51_PIN_NANDF_CS4, pad | PAD_CTL_47K_PU);
		mxc_iomux_set_pad(MX51_PIN_NANDF_CS5, pad | PAD_CTL_47K_PU);
		mxc_iomux_set_pad(MX51_PIN_NANDF_CS6, pad | PAD_CTL_100K_PU);
		break;
	default:
	  interface_esdhc = status = -1;
	  break;
	}

	return status;
}

int board_mmc_init(void)
{
	if (!esdhc_gpio_init(CONFIG_MMC_BOOTFLASH))
	    return fsl_esdhc_initialize(gd->bd, &mmc_cfg);
	else
		return -1;
}

#endif

#if defined(CONFIG_MXC_KPD)
int setup_mxc_kpd(void)
{
	mxc_request_iomux(MX51_PIN_KEY_COL0, IOMUX_CONFIG_ALT0);
	mxc_request_iomux(MX51_PIN_KEY_COL1, IOMUX_CONFIG_ALT0);
	mxc_request_iomux(MX51_PIN_KEY_COL2, IOMUX_CONFIG_ALT0);
	mxc_request_iomux(MX51_PIN_KEY_COL3, IOMUX_CONFIG_ALT0);
	mxc_request_iomux(MX51_PIN_KEY_COL4, IOMUX_CONFIG_ALT0);
	mxc_request_iomux(MX51_PIN_KEY_COL5, IOMUX_CONFIG_ALT0);
	mxc_request_iomux(MX51_PIN_KEY_ROW0, IOMUX_CONFIG_ALT0);
	mxc_request_iomux(MX51_PIN_KEY_ROW1, IOMUX_CONFIG_ALT0);
	mxc_request_iomux(MX51_PIN_KEY_ROW2, IOMUX_CONFIG_ALT0);
	mxc_request_iomux(MX51_PIN_KEY_ROW3, IOMUX_CONFIG_ALT0);

	return 0;
}
#endif

int board_init(void)
{
	setup_boot_device();

#ifdef CONFIG_HW_WATCHDOG
        /* set the timeout to the max number of ticks
         * and WDZST
         * leave other settings the same */
        WDOG_WCR = 0xff01 | (WDOG_WCR & 0xff);
        WDOG_WMCR = 0; /* Power Down Counter of WDOG is disabled. */
#endif
	setup_soc_rev();

	gd->bd->bi_arch_number = MACH_TYPE_MX51_BABBAGE;	/* board id for linux */
	/* address of boot parameters */
	gd->bd->bi_boot_params = PHYS_SDRAM_1 + 0x100;

	setup_uart();

#ifdef CONFIG_MXC_NAND
	setup_nfc();
#endif

#ifdef CONFIG_MXC_FEC
	setup_expio();
	setup_fec();
#endif
	return 0;
}

#ifdef BOARD_LATE_INIT

inline int check_boot_mode(void) 
{
	char boot_mode[20];
	char boot_cmd[20];

#if defined(CONFIG_CMD_IDME)
	if (idme_get_var("bootmode", boot_mode, 20)) 
#endif
	{
	    return -1;
	}

	boot_cmd[0] = 0;

	if (!strncmp(boot_mode, "update", 6)) {
	    sprintf(boot_cmd, "bootm 0x%x", CONFIG_MMC_BOOTFLASH_UPDATE_ADDR);
	} else if (!strncmp(boot_mode, "diags", 5)) {
	    sprintf(boot_cmd, "bootm 0x%x", CONFIG_MMC_BOOTFLASH_DIAGS_ADDR);
	} else if (!strncmp(boot_mode, "fastboot", 8)) {
	    strcpy(boot_cmd, "fastboot");
	} else {
	    return 0;
	}
	
	setenv("bootcmd", boot_cmd);

	return 0;
}

int board_late_init(void)
{

	/* figure out which partition to boot */
	check_boot_mode();
	
#if defined(CONFIG_PMIC_13892) && defined(CONFIG_GADGET_FILE_STORAGE)
	{
	    unsigned short voltage;
	    int ret;

	    pmic_init();

	    ret = pmic_adc_read_voltage(&voltage);
	    if (ret) {
		printf("Battery voltage: %d mV\n\n", voltage);
	    } else {
		printf("Battery voltage read fail!\n\n");
	    }

	    /* stop boot if battery is too low */
	    while (voltage <= CONFIG_BOOT_HALT_VOLTAGE) {

		printf("Battery voltage too low.  Please plug in a charger\n");
		ret = file_storage_enable(CONFIG_BOOT_CONTINUE_VOLTAGE);
		if (ret) {
#ifdef CONFIG_CMD_HALT
		    printf("Can't enable charger.. shutting down\n");
		    board_power_off();
#endif
		}

		ret = pmic_adc_read_voltage(&voltage);
		if (ret) {
		    printf("Battery voltage: %d mV\n", voltage);
		} else {
		    printf("Battery voltage read fail!\n");
		}
	    }
	}
#endif
	return 0;
}
#endif

int checkboard(void)
{
	const char *sn, *rev;
	printf("Board: Banjo MX51 ver ");

	if (system_rev & CHIP_REV_3_0) {
		printf("3.0 [");
	} else if (system_rev & CHIP_REV_2_5) {
		printf("2.5 [");
	} else if (system_rev & CHIP_REV_2_0) {
		printf("2.0 [");
	} else if (system_rev & CHIP_REV_1_1) {
		printf("1.1 [");
	} else {
		printf("1.0 [");
	}

	switch (__REG(SRC_BASE_ADDR + 0x8)) {
	case 0x0001:
		printf("POR");
		break;
	case 0x0009:
		printf("RST");
		break;
	case 0x0010:
	case 0x0011:
		printf("WDOG");
		break;
	default:
		printf("unknown");
	}
	printf("]\n");

	printf("Boot Device: ");
	switch (get_boot_device()) {
	case NAND_BOOT:
		printf("NAND\n");
		break;
	case SPI_NOR_BOOT:
		printf("SPI NOR\n");
		break;
	case MMC_BOOT:
		printf("MMC\n");
		break;
	case UNKNOWN_BOOT:
	default:
		printf("UNKNOWN\n");
		break;
	}
	
	/* serial number and board id */
	sn = (char *) get_board_serial16();
	rev = (char *) get_board_id16();

	if (rev)
		printf ("Board Id: %.*s\n", 16, rev);

	if (sn)
		printf ("S/N: %.*s\n", 16, sn);

	return 0;
}

#ifdef CONFIG_HW_WATCHDOG
void hw_watchdog_reset(void)
{
        /* service the watchdog */
        WDOG_WSR = 0x5555;
        WDOG_WSR = 0xaaaa;
}
#endif

inline int check_post_mode(void) 
{
	char post_mode[20];

#if defined(CONFIG_CMD_IDME)
	if (idme_get_var("postmode", post_mode, 20)) 
#endif
	{
		return -1;
	}

	if (!strncmp(post_mode, "normal", 6)) {
		setenv("post_hotkeys", "0");
	} else if (!strncmp(post_mode, "slow", 4)) {
		setenv("post_hotkeys", "1");
	} else if (!strncmp(post_mode, "factory", 7)) {
		setenv("bootdelay", "-1");
#if defined(CONFIG_CMD_IDME) && defined(CONFIG_FOR_FACTORY)
	} else if (!board_info_valid(serial_number) || !board_info_valid(board_id)) {
	    printf("ERROR: Serial number or board id is not set!\n\n");
	    setenv("bootdelay", "-1");
#endif
	}

	return 0;
}

#if defined(CONFIG_HANG_FEEDBACK)
#if defined(CONFIG_PMIC_13892)
#define led_init()    {pmic_init();}
#define led_2_off()   {pmic_enable_green_led(0);}
#define led_2_on()    {pmic_enable_green_led(1);}
#endif
#define morse_delay(t)	{{unsigned long msec=(t*100); while (msec--) { udelay(1000);}}}
#define short_gap()  {morse_delay(2);}
#define gap()  {led_2_off(); morse_delay(1);}
#define dit()  {led_2_on();  morse_delay(1); gap();}
#define dah()  {led_2_on();  morse_delay(3); gap();}
void hang_feedback (void)
{
    led_init();
    dit(); dit(); dit(); short_gap(); /* Morse Code S */
    dah(); dah(); dah(); short_gap(); /* Morse Code O */
    dit(); dit(); dit(); short_gap(); /* Morse Code S */
    if (!ctrlc()) {
#ifdef CONFIG_CMD_HALT
      board_power_off(); 
#endif
      while (1) {};
    }
}
#endif
#ifdef CONFIG_POST
int post_hotkeys_pressed(void)
{
    char *value;
    int ret;

    check_post_mode();

    ret = ctrlc();
    if (!ret) {
        value = getenv("post_hotkeys");
        if (value != NULL)
	    ret = simple_strtoul(value, NULL, 10);
    }
    return ret;
}
#endif

#if defined(CONFIG_POST) || defined(CONFIG_LOGBUFFER)
void post_word_store (ulong a)
{
	volatile ulong *save_addr =
		(volatile ulong *)(CONFIG_SYS_POST_WORD_ADDR);
	*save_addr = a;
}
ulong post_word_load (void)
{
  volatile ulong *save_addr =
		(volatile ulong *)(CONFIG_SYS_POST_WORD_ADDR);
  return *save_addr;
}
#endif

#ifdef CONFIG_LOGBUFFER
unsigned long logbuffer_base(void)
{
  /* OOPS_SAVE_BASE + PAGE_SIZE in linux/include/asm-arm/arch/boot_globals.h */
  return CONFIG_SYS_SDRAM_BASE + (2*4096);
}
#endif
