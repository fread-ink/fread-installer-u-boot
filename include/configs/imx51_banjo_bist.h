/*
 * Copyright (C) 2007, Guennadi Liakhovetski <lg@denx.de>
 *
 * (C) Copyright 2009 Freescale Semiconductor, Inc.
 *
 * Configuration settings for the MX51-3Stack Freescale board.
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

#include <asm/arch/mx51.h>

 /* High Level Configuration Options */
#define CONFIG_ARMV7		1	/* This is armv7 Cortex-A8 CPU core */
#define CONFIG_L2_OFF

#define CONFIG_MXC		1
#define CONFIG_MX51		1
#define CONFIG_FLASH_HEADER	1
#define CONFIG_FLASH_HEADER_OFFSET 0x400
#define CONFIG_FLASH_HEADER_BARKER 0xB1

#define CONFIG_SKIP_RELOCATE_UBOOT

#define CONFIG_MX51_HCLK_FREQ	24000000	/* RedBoot says 26MHz */

#define CONFIG_ARCH_CPU_INIT

#define CONFIG_DISPLAY_CPUINFO
#define CONFIG_DISPLAY_BOARDINFO

#define BOARD_LATE_INIT

#define CONFIG_PANIC_HANG  /* Do not reboot if a panic occurs */
#define CONFIG_HANG_FEEDBACK /* Flash LEDs before hang */

#define CONFIG_CMDLINE_TAG			1	/* enable passing of ATAGs */
#define CONFIG_REVISION_TAG			1
#define CONFIG_SETUP_MEMORY_TAGS	1
#define CONFIG_INITRD_TAG			1
#define CONFIG_SERIAL16_TAG			1
#define CONFIG_REVISION16_TAG		1
#define CONFIG_POST_TAG				1

/*
 * Size of malloc() pool
 */
#define CONFIG_SYS_MALLOC_LEN		(CONFIG_ENV_SIZE + 20 * 1024)
/* size in bytes reserved for initial data */
#define CONFIG_SYS_GBL_DATA_SIZE	128

#define CONFIG_SYS_GBL_DATA_OFFSET	(TEXT_BASE - CONFIG_SYS_MALLOC_LEN - CONFIG_SYS_GBL_DATA_SIZE)
#define CONFIG_SYS_POST_WORD_ADDR	(CONFIG_SYS_GBL_DATA_OFFSET - 0x4)
#define CONFIG_SYS_INIT_SP_OFFSET	CONFIG_SYS_POST_WORD_ADDR

/*
 * Hardware drivers
 */
#define CONFIG_MX51_UART	1
#define CONFIG_MX51_UART1	1
#define CONFIG_MX51_GPIO	1

/*
 * SPI Configs
 */
#define CONFIG_IMX_SPI
#define CONFIG_IMX_CSPI
#define CONFIG_IMX_ECSPI
#define CONFIG_IMX_SPI_PMIC_BUS 1
#define CONFIG_IMX_SPI_PMIC_CS  0
#define MAX_SPI_BYTES		(64 * 4)

#define CONFIG_PMIC_13892	1

/*
 * MMC Configs
 */
#define CONFIG_MMC		1
#define CONFIG_GENERIC_MMC	1
#define CONFIG_IMX_MMC		1
#define CONFIG_SYS_FSL_ESDHC_NUM	1
#define CONFIG_MMC_BOOTFLASH	0
#define CONFIG_MMC_BOOTFLASH_ADDR		0x41000
#define CONFIG_MMC_BOOTFLASH_UPDATE_ADDR	0x41000
#define CONFIG_MMC_BOOTFLASH_DIAGS_ADDR		0x41000
#define CONFIG_MMC_BOOTFLASH_SIZE		0x340000
#define CONFIG_MMC_USERDATA_ADDR		0x3f000
#define CONFIG_MMC_USERDATA_SIZE		0x1400
#define CONFIG_MMC_BIST_ADDR			0x14400
#define CONFIG_MMC_BIST_SIZE			0x2C000

/* crc32 Buffer*/
#define CRC32_BUFFER        0x02000000 + PHYS_SDRAM_1
#define CRC32_BUFFER_SIZE 	0x00400000
#define CRC32_CHECK_UBOOT	0x00000010	/* check crc32 on u-boot.bin */
#define CRC32_CHECK_UIMAGE	0x00000020	/* check crc32 on uImage */
#define CRC32_CHECK_ROOTFS	0x00000040	/* check crc32 on rootfs.img */
#define CRC32_CHECK_MBR		0x00000080	/* check crc32 on mbr-xxx-bin */

/*
 * USB Configs
 */
#define CONFIG_USB_DEVICE		1
#define CONFIG_DRIVER_FSLUSB		1
#define CONFIG_GADGET_FASTBOOT		1
#define CONFIG_GADGET_FILE_STORAGE	1

#define CONFIG_USBD_MANUFACTURER "Amazon"
#define CONFIG_USBD_PRODUCT_NAME "Kindle"

#define CONFIG_USBD_VENDORID			0x1949
#define CONFIG_USBD_PRODUCTID_FASTBOOT		0xd0d0
#define CONFIG_USBD_PRODUCTID_FILE_STORAGE	0x0003
#define CONFIG_FASTBOOT_MAX_DOWNLOAD_LEN	(400*1024*1024)
#define CONFIG_FASTBOOT_TEMP_BUFFER		0x97700000

#define CONFIG_BOOT_HALT_VOLTAGE	3400	/* 3.4V */
#define CONFIG_BOOT_CONTINUE_VOLTAGE	3600	/* 3.6V */	
#define CONFIG_BOOT_AUTOCHG_VOLTAGE	3800	/* 3.8V */	

/*
 * I2C drivers
 */
#define CONFIG_MX51_I2C		1
#define CONFIG_SYS_I2C_INIT_BOARD	1
#define CONFIG_HARD_I2C		1		/* I2C with hardware support	*/
#undef	CONFIG_SOFT_I2C				/* I2C bit-banged		*/
#define CONFIG_I2C_MULTI_BUS
#define CONFIG_SYS_I2C_SPEED	100000		/* I2C speed and slave address	*/
#define CONFIG_SYS_I2C_SLAVE	0x7F
//#undef I2C_ADDR_LIST
//#define I2C_ADDR_LIST		{ 0, 0x18 }	//default audio codec

/* allow to overwrite serial and ethaddr */
#define CONFIG_ENV_OVERWRITE
#define CONFIG_CONS_INDEX		1
#define CONFIG_BAUDRATE			115200
#define CONFIG_SYS_BAUDRATE_TABLE	{9600, 19200, 38400, 57600, 115200}

/***********************************************************
 * Command definition
 ***********************************************************/

// Standard commands
#define CONFIG_CMD_BOOTD	/* bootd			*/
#define CONFIG_CMD_CONSOLE	/* coninfo			*/
#define CONFIG_CMD_ECHO		/* echo arguments		*/
#define CONFIG_CMD_IMI		/* iminfo			*/
#define CONFIG_CMD_ITEST	/* Integer (and string) test	*/
#define CONFIG_CMD_LOADB	/* loadb			*/
#define CONFIG_CMD_LOADS	/* loads			*/
#define CONFIG_CMD_MEMORY	/* md mm nm mw cp cmp crc base loop mtest */
#define CONFIG_CMD_MISC		/* Misc functions like sleep etc*/
#define CONFIG_CMD_RUN		/* run command in env variable	*/
#define CONFIG_CMD_SOURCE	/* "source" command support	*/
#define CONFIG_CMD_ENV
#define CONFIG_CMD_MMC
#define CONFIG_CMD_PANIC
#define CONFIG_XYZMODEM
#define CONFIG_SRECORD
#define CONFIG_LOOPW

/* Lab 126 cmds */
#define CONFIG_CMD_GADGET	1
#define CONFIG_CMD_I2C		1

#define CONFIG_CMD_PMIC		1
#define CONFIG_CMD_IDME		1
#define CONFIG_CMD_CRC 		1

/*
 * Boot variables 
 */
#define XMK_STR(x)	#x
#define MK_STR(x)	XMK_STR(x)

#define CONFIG_BOOTDELAY	-1

#define CONFIG_LOADADDR		0x97840000	/* loadaddr env var */
#define CONFIG_BISTADDR		0x97800000

#define CONFIG_BISTCMD		1
#define CONFIG_BISTCMD_LOCATION 0x97600000
#define CONFIG_BISTCMD_MAGIC	0xBC 

#define	CONFIG_EXTRA_ENV_SETTINGS \
    "testmem=mtest 0x90000000 0x976FFFFF 0 1 2; mtest 0x97900000 0xAFFFFFFF 0 1 2\0" \
    "bootcmd=bootm " MK_STR(CONFIG_MMC_BOOTFLASH_ADDR) "\0" \
    "failbootcmd=panic\0" \
    "post_hotkeys=0\0" \
    "loglevel=5\0"

/*
 * Miscellaneous configurable options
 */
#define CONFIG_SYS_LONGHELP		/* undef to save memory */
#define CONFIG_SYS_PROMPT		"bist > "
#define CONFIG_AUTO_COMPLETE

#define CONFIG_SYS_CBSIZE		256	/* Console I/O Buffer Size */
/* Print Buffer Size */
#define CONFIG_SYS_PBSIZE (CONFIG_SYS_CBSIZE + sizeof(CONFIG_SYS_PROMPT) + 16)
#define CONFIG_SYS_MAXARGS	16	/* max number of command args */
#define CONFIG_SYS_BARGSIZE CONFIG_SYS_CBSIZE /* Boot Argument Buffer Size */

#define CONFIG_FOR_FACTORY

#define CONFIG_SYS_ORG_MEMTEST      /* Original (not so) quickie memory test */
#define CONFIG_SYS_ALT_MEMTEST      /* Newer data, address, integrity test */
#define CONFIG_SYS_MEMTEST_SCRATCH  0x10000000      /* Internal RAM */
#define CONFIG_SYS_MEMTEST_START    PHYS_SDRAM_1	/* memtest works on */
#define CONFIG_SYS_MEMTEST_END      (PHYS_SDRAM_1 + PHYS_SDRAM_1_SIZE - 1)
#define CONFIG_SYS_MEMPROTECT_START (CONFIG_SYS_INIT_SP_OFFSET & 0xFFF00000)
#define CONFIG_SYS_MEMPROTECT_END   ((_bss_end & 0xFFF00000) + 0x00100000 - 1)

#define CONFIG_CMD_DIAG
#define CONFIG_POST         (CONFIG_SYS_POST_MMC_CRC32 | \
                             CONFIG_SYS_POST_INVENTORY | \
                             CONFIG_SYS_POST_GPIO | \
                             CONFIG_SYS_POST_I2C | \
                             CONFIG_SYS_POST_USB | \
                             CONFIG_SYS_POST_FAIL)

#undef	CONFIG_SYS_CLKS_IN_HZ		/* everything, incl board info, in Hz */

#define CONFIG_SYS_LOAD_ADDR		CONFIG_LOADADDR

#define CONFIG_SYS_HZ				1000

#define CONFIG_CMDLINE_EDITING	1
#define CONFIG_HW_WATCHDOG 1

/*-----------------------------------------------------------------------
 * Stack sizes
 *
 * The stack sizes are set up in start.S using the settings below
 */
#define CONFIG_STACKSIZE	(30 * 1024) /* regular stack */

/*-----------------------------------------------------------------------
 * Physical Memory Map
 */
#define CONFIG_NR_DRAM_BANKS	1
#define PHYS_SDRAM_1		CSD0_BASE_ADDR
#define PHYS_SDRAM_1_SIZE	(512 * 1024 * 1024)

#define CONFIG_SYS_SDRAM_BASE       PHYS_SDRAM_1
#define CONFIG_SYS_SDRAM_SIZE       PHYS_SDRAM_1_SIZE

/*-----------------------------------------------------------------------
 * FLASH and environment organization
 */
#define CONFIG_SYS_NO_FLASH

#define CONFIG_ENV_SECT_SIZE    (4 * 1024)
#define CONFIG_ENV_SIZE         CONFIG_ENV_SECT_SIZE

#if defined(CONFIG_FSL_ENV_IN_MMC)
	#define CONFIG_ENV_IS_IN_MMC	1
	#define CONFIG_ENV_OFFSET	(768 * 1024)
#elif defined(CONFIG_FSL_ENV_IN_SF)
	#define CONFIG_ENV_IS_IN_SPI_FLASH	1
	#define CONFIG_ENV_SPI_CS		1
	#define CONFIG_ENV_OFFSET       (768 * 1024)
#else
	#define CONFIG_ENV_IS_NOWHERE	1
#endif

#define CONFIG_NUM_PARTITIONS	4

#ifndef __ASSEMBLY__
typedef struct partition_info_t {
    const char *name;
    unsigned int address;
    unsigned int size;
} partition_info_t;

static const struct partition_info_t partition_info[CONFIG_NUM_PARTITIONS] = {
    {
	.name = "bootloader",
	.address = 0x400,
	.size = 0x3F000, // 252 kB
    },
    {
	.name = "userdata",
	.address = CONFIG_MMC_USERDATA_ADDR,
	.size = CONFIG_MMC_USERDATA_SIZE,  // 5 kB
    },
    {
	.name = "kernel",
	.address = CONFIG_MMC_BOOTFLASH_ADDR,
	.size = CONFIG_MMC_BOOTFLASH_SIZE,  // ~3 MB
    },
    {
	.name = "system",
	.address = 0x3C1000,
	.size = 0x28A00000,  // 650 MB
    }
};

#define CONFIG_NUM_NV_VARS 9

typedef struct nvram_t {
    const char *name;
    unsigned int offset;
    unsigned int size;
} nvram_t;

static const struct nvram_t nvram_info[CONFIG_NUM_NV_VARS] = {
    {
	.name = "boardid",
	.offset = 0,
	.size = 16,
    },
    {
	.name = "serial",
	.offset = 0x10,
	.size = 16,
    },
    {
	.name = "accel",
	.offset = 0x20,
	.size = 16,
    },
    {
	.name = "mac",
	.offset = 0x30,
	.size = 12,
    },
    {
	.name = "sec",
	.offset = 0x40,
	.size = 20,
    },
    {
	.name = "pcbsn",
	.offset = 0x60,
	.size = 16,
    },
    {
	.name = "config",
	.offset = 0x70,
	.size = 12,
    },
    {
	.name = "bootmode",
	.offset = 0x1000,
	.size = 16,
    },
    {
	.name = "postmode",
	.offset = 0x1010,
	.size = 16,
    },
};

#endif

#endif				/* __CONFIG_H */
