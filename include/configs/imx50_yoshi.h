/*
 * Copyright (C) 2010 Freescale Semiconductor, Inc.
 *
 * Configuration settings for the MX50-ARM2 Freescale board.
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; either version 2 of
 * the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.	 See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston,
 * MA 02111-1307 USA
 */

#ifndef __CONFIG_H
#define __CONFIG_H

#include <asm/arch/mx50.h>

#define CONFIG_MD5

 /* High Level Configuration Options */
#define CONFIG_ARMV7		1	/* This is armv7 Cortex-A8 CPU core */
#define CONFIG_MXC
#define CONFIG_MX50
#define CONFIG_FLASH_HEADER
#define CONFIG_FLASH_HEADER_OFFSET 0x400

#define CONFIG_SKIP_RELOCATE_UBOOT

#define CONFIG_ARCH_CPU_INIT

#define CONFIG_MX50_HCLK_FREQ	24000000
#define CONFIG_SYS_PLL2_FREQ    400
#define CONFIG_SYS_AHB_PODF     3
#define CONFIG_SYS_AXIA_PODF    0
#define CONFIG_SYS_AXIB_PODF    1

//#define CONFIG_DISPLAY_CPUINFO
//#define CONFIG_DISPLAY_BOARDINFO

#define CONFIG_SYS_64BIT_VSPRINTF

#define BOARD_LATE_INIT
#define CONFIG_IRAM_BOOT	1
#define CONFIG_ZQ_CALIB		1

#define CONFIG_PANIC_HANG  /* Do not reboot if a panic occurs */
#if 0 /* BEN TODO */
#define CONFIG_HANG_FEEDBACK /* Flash LEDs before hang */
#endif

#define CONFIG_CMDLINE_TAG		1	/* enable passing of ATAGs */
#define CONFIG_REVISION_TAG		1
#define CONFIG_SETUP_MEMORY_TAGS	1
#define CONFIG_INITRD_TAG		1
#define CONFIG_SERIAL16_TAG		1
#define CONFIG_REVISION16_TAG		1
#define CONFIG_POST_TAG			1
#define CONFIG_MACADDR_TAG		1
#define CONFIG_BOOTMODE_TAG		1

#define CONFIG_DSN_LEN	16
#define CONFIG_PCBA_LEN	16

/*
 * Size of malloc() pool
 */
#define CONFIG_SYS_MALLOC_LEN		(CONFIG_ENV_SIZE + 8 * 1024)
/* size in bytes reserved for initial data */
#define CONFIG_SYS_GBL_DATA_SIZE	128

#define CONFIG_SYS_GBL_DATA_OFFSET	(TEXT_BASE - CONFIG_SYS_MALLOC_LEN - CONFIG_SYS_GBL_DATA_SIZE)
#define CONFIG_SYS_POST_WORD_ADDR	(CONFIG_SYS_GBL_DATA_OFFSET - 0x4)
#define CONFIG_SYS_INIT_SP_OFFSET	CONFIG_SYS_POST_WORD_ADDR

/*
 * Hardware drivers
 */
#define CONFIG_MX50_UART	1
#define CONFIG_MX50_UART1	1
#define CONFIG_MX50_GPIO	1

/*
 * SPI Configs
 */
#define CONFIG_IMX_SPI
#define CONFIG_IMX_CSPI
#define CONFIG_IMX_ECSPI
#define MAX_SPI_BYTES        (8 * 4)
#define CONFIG_IMX_SPI_PMIC_BUS 3
#define CONFIG_IMX_SPI_PMIC_CS    0
#define CONFIG_PMIC        1
#define CONFIG_PMIC_13892    1

/*
 * MMC Configs
 */
#define CONFIG_MMC			1
#define CONFIG_GENERIC_MMC		1
#define CONFIG_IMX_MMC			1
#define CONFIG_SYS_FSL_ESDHC_DMA	1
#define CONFIG_SYS_FSL_ESDHC_NUM        4
#define CONFIG_MMC_BOOTFLASH		0
#define CONFIG_MMC_BOOTFLASH_ADDR	0x41000
#define CONFIG_MMC_BOOTFLASH_SIZE	(14*1024*1024) /* 14 MiB */
#define CONFIG_MMC_BOOTDIAGS_ADDR	0xE41000
#define CONFIG_MMC_USERDATA_ADDR	0x3F000
#define CONFIG_MMC_USERDATA_SIZE	(5*1024)
#define CONFIG_MMC_BIST_ADDR		(120*1024)
#define CONFIG_MMC_BIST_SIZE		(256*1024)
/* detect whether ESDHC1 or ESDHC3 is boot device */
/* #define CONFIG_DYNAMIC_MMC_DEVNO */
#define CONFIG_BOOT_PARTITION_ACCESS
#define CONFIG_BOOT_FROM_PARTITION	1

#define CONFIG_MMC_MAX_TRANSFER_SIZE    (0xFFFF * 512)

/*
 * USB Configs
 */
#define CONFIG_USB_DEVICE        1
#define CONFIG_DRIVER_FSLUSB        1
#define CONFIG_GADGET_FASTBOOT        1

#define CONFIG_USBD_MANUFACTURER "Amazon"
#define CONFIG_USBD_PRODUCT_NAME "Kindle"

#define CONFIG_USBD_VENDORID            0x1949
#define CONFIG_USBD_PRODUCTID_FASTBOOT        0xd0d0
#define CONFIG_FASTBOOT_MAX_DOWNLOAD_LEN    ((get_dram_size()) - (2*1024*1024) - (CONFIG_FASTBOOT_TEMP_BUFFER - CONFIG_SYS_SDRAM_BASE))
#define CONFIG_FASTBOOT_TEMP_BUFFER        0x7A000000
//#define CONFIG_CMD_GADGET 1

#define CONFIG_BOOT_HALT_VOLTAGE	3400	/* 3.4V */
#define CONFIG_BOOT_CONTINUE_VOLTAGE	3600	/* 3.6V */	
#define CONFIG_BOOT_AUTOCHG_VOLTAGE	3800	/* 3.8V */	

/* allow to overwrite serial and ethaddr */
#define CONFIG_ENV_OVERWRITE
#define CONFIG_CONS_INDEX		1
#define CONFIG_BAUDRATE			115200
#define CONFIG_SYS_BAUDRATE_TABLE	{9600, 19200, 38400, 57600, 115200}

/***********************************************************
 * Command definition
 ***********************************************************/

//#define CONFIG_CMD_BOOTD	/* bootd			*/
//#define CONFIG_CMD_RUN		/* run command in env variable	*/
//#define CONFIG_CMD_LOG

/* Lab 126 cmds */
//#define CONFIG_CMD_BIST		1
//#define CONFIG_CMD_PMIC		1
#define CONFIG_CMD_IDME		1
//#define CONFIG_CMD_HALT		1

#define CONFIG_IDME_UPDATE		1
#define CONFIG_IDME_UPDATE_ADDR		0x3f000
#define CONFIG_IDME_UPDATE_MAGIC	"abcdefghhgfedcba"
#define CONFIG_IDME_UPDATE_MAGIC_SIZE	16

/*#define CONFIG_CMD */
#define CONFIG_REF_CLK_FREQ CONFIG_MX50_HCLK_FREQ

#undef CONFIG_CMD_IMLS

/*
 * Boot variables 
 */
#define XMK_STR(x)	#x
#define MK_STR(x)	XMK_STR(x)

#if defined(DEVELOPMENT_MODE)
#define CONFIG_BOOTDELAY	3
#else
#define CONFIG_BOOTDELAY	1
#endif

#define CONFIG_LOADADDR		0x70800000	/* loadaddr env var */
/*#define CONFIG_RD_LOADADDR	(CONFIG_LOADADDR + 0x300000)
#define CONFIG_BISTADDR		0x79800000

#define CONFIG_BISTCMD_LOCATION (CONFIG_BISTADDR - 0x80000)
#define CONFIG_BISTCMD_MAGIC	0xBC 

#define	CONFIG_EXTRA_ENV_SETTINGS \
    "bootcmd=bootm " MK_STR(CONFIG_MMC_BOOTFLASH_ADDR) "\0" \
    "failbootcmd=panic\0" \
    "post_hotkeys=0\0" \
    "loglevel=5\0" \
    "bootargs_diags=setenv bootargs consoleblank=0 rootwait ro ip=off root=/dev/mmcblk0p2 quiet eink=fslepdc\0" \
    "bootcmd_diags=run bootargs_diags ; bootm " MK_STR(CONFIG_MMC_BOOTDIAGS_ADDR) "\0" \
    "bootcmd_factory=bist halt\0" \
    "bootcmd_fastboot=bist fastboot\0"*/

/*
 * Miscellaneous configurable options
 */
#undef CONFIG_SYS_LONGHELP		/* undef to save memory */
#define CONFIG_SYS_PROMPT		"uboot > "
#undef CONFIG_AUTO_COMPLETE

#define CONFIG_SYS_CBSIZE		256	/* Console I/O Buffer Size */
/* Print Buffer Size */
#define CONFIG_SYS_PBSIZE (CONFIG_SYS_CBSIZE + sizeof(CONFIG_SYS_PROMPT) + 16)
#define CONFIG_SYS_MAXARGS	16	/* max number of command args */
#define CONFIG_SYS_BARGSIZE CONFIG_SYS_CBSIZE /* Boot Argument Buffer Size */

#define CONFIG_SYS_ORG_MEMTEST      /* Original (not so) quickie memory test */
#define CONFIG_SYS_ALT_MEMTEST      /* Newer data, address, integrity test */
#define CONFIG_SYS_MEMTEST_SCRATCH  IRAM_BASE_ADDR      /* Internal RAM */
#define CONFIG_SYS_MEMTEST_START    PHYS_SDRAM_1	/* memtest works on */
#define CONFIG_WDOG_PRINTK_SIZE	    (4096 * 2)
#define CONFIG_SYS_MEMTEST_END      (PHYS_SDRAM_1 + get_dram_size() - 1 - CONFIG_WDOG_PRINTK_SIZE)

//#define CONFIG_LOGBUFFER

#define CONFIG_POST         (CONFIG_SYS_POST_MEMORY | CONFIG_SYS_POST_FAIL)
//                             CONFIG_SYS_POST_FAIL | CONFIG_SYS_POST_MMC_CRC32)

#define DYNAMIC_CRC_TABLE 1


#undef	CONFIG_SYS_CLKS_IN_HZ		/* everything, incl board info, in Hz */

#define CONFIG_SYS_LOAD_ADDR		CONFIG_LOADADDR

#define CONFIG_SYS_HZ				1000

//#define CONFIG_CMDLINE_EDITING	1

/*-----------------------------------------------------------------------
 * Physical Memory Map
 */
#define CONFIG_NR_DRAM_BANKS	1
#define PHYS_SDRAM_1		CSD0_BASE_ADDR
#define CONFIG_SYS_SDRAM_BASE	PHYS_SDRAM_1
#define CONFIG_SYS_SDRAM_SIZE	get_dram_size()

/*-----------------------------------------------------------------------
 * FLASH and environment organization
 */
#define CONFIG_SYS_NO_FLASH

#define CONFIG_ENV_SECT_SIZE    (1 * 1024)
#define CONFIG_ENV_SIZE         CONFIG_ENV_SECT_SIZE

#if defined(CONFIG_FSL_ENV_IN_NAND)
	#define CONFIG_ENV_IS_IN_NAND 1
	#define CONFIG_ENV_OFFSET	0x100000
#elif defined(CONFIG_FSL_ENV_IN_MMC)
	#define CONFIG_ENV_IS_IN_MMC	1
	#define CONFIG_ENV_OFFSET	(768 * 1024)
#elif defined(CONFIG_FSL_ENV_IN_SF)
	#define CONFIG_ENV_IS_IN_SPI_FLASH	1
	#define CONFIG_ENV_SPI_CS		1
	#define CONFIG_ENV_OFFSET       (768 * 1024)
#else
	#define CONFIG_ENV_IS_NOWHERE	1
#endif

#ifndef __ASSEMBLY__
#include <asm/arch/mx50_yoshi_board.h>
#endif

#endif				/* __CONFIG_H */


