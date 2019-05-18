/*
 * Copyright (C) 2007, Guennadi Liakhovetski <lg@denx.de>
 *
 * (C) Copyright 2009-2010 Freescale Semiconductor, Inc.
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
#include <asm/arch/mx50.h>
#include <asm/arch/mx50_pins.h>
#include <asm/arch/iomux.h>
#include <asm/errno.h>
#include <linux/ctype.h>

#if defined(CONFIG_IMX_SPI)
#include <imx_spi.h>
#endif

#if CONFIG_MX50_I2C
#include <mx50_i2c.h>
#endif

#ifdef CONFIG_MMC
#include <mmc.h>
#include <fsl_esdhc.h>
#endif

#ifdef CONFIG_ARCH_MMU
#include <asm/mmu.h>
#include <asm/arch/mmu.h>
#endif

#ifdef CONFIG_CMD_CLOCK
#include <asm/clock.h>
#endif

#if defined (CONFIG_CMD_IDME)
#include <idme.h>
#endif

#if defined(CONFIG_GADGET_FILE_STORAGE)
#include <usb/file_storage.h>
#endif

#if defined(CONFIG_PMIC)
#include <pmic.h>
#endif

#include <usb/fastboot.h>

DECLARE_GLOBAL_DATA_PTR;

unsigned int g_uart_addr = UART1_BASE_ADDR;

static u32 system_rev;
static enum boot_device boot_dev;

/* board id and serial number. */
static u8 serial_number[CONFIG_DSN_LEN+1];
static u8 board_id[CONFIG_PCBA_LEN+1];

static inline void setup_boot_device(void)
{
	uint soc_sbmr = readl(SRC_BASE_ADDR + 0x4);
	uint bt_mem_ctl = (soc_sbmr & 0x000000FF) >> 4 ;
	uint bt_mem_type = (soc_sbmr & 0x00000008) >> 3;

	switch (bt_mem_ctl) {
	case 0x0:
		if (bt_mem_type)
			boot_dev = ONE_NAND_BOOT;
		else
			boot_dev = WEIM_NOR_BOOT;
		break;
	case 0x2:
		if (bt_mem_type)
			boot_dev = SATA_BOOT;
		else
			boot_dev = PATA_BOOT;
		break;
	case 0x3:
		if (bt_mem_type)
			boot_dev = SPI_NOR_BOOT;
		else
			boot_dev = I2C_BOOT;
		break;
	case 0x4:
	case 0x5:
		boot_dev = SD_BOOT;
		break;
	case 0x6:
	case 0x7:
		boot_dev = MMC_BOOT;
		break;
	case 0x8 ... 0xf:
		boot_dev = NAND_BOOT;
		break;
	default:
		boot_dev = UNKNOWN_BOOT;
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
	int reg = __REG(ROM_SI_REV);

	switch (reg) {
	case 0x10:
		system_rev = 0x50000 | CHIP_REV_1_0;
		break;
	case 0x11:
		system_rev = 0x50000 | CHIP_REV_1_1_1;
		break;
	default:
		system_rev = 0x50000 | CHIP_REV_1_1_1;
	}
}

static inline void setup_board_rev(int rev)
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
  if (idme_get_var("pcbsn", (char *) board_id, sizeof(board_id))) 
#endif
    {
      /* not found: clean up garbage characters. */
      memset(board_id, 0, sizeof(board_id));
    }

#if defined(CONFIG_CMD_IDME)
  if (idme_get_var("serial", (char *) serial_number, sizeof(serial_number))) 
#endif
    {
      /* not found: clean up garbage characters. */
      memset(serial_number, 0, sizeof(serial_number));
    }

  return 0;
}

int board_info_valid (u8 *info, int len)
{
  int i;

  for (i = 0; i < len; i++) {
    if ((info[i] < '0') && 
        (info[i] > '9') &&
        (info[i] < 'A') &&
        (info[i] > 'Z'))
	    return 0;
  }

  return 1;
}

/*************************************************************************
 * get_board_serial() - setup to pass kernel serial number information
 *      return: alphanumeric containing the serial number.
 *************************************************************************/
const u8 *get_board_serial(void)
{
  if (!board_info_valid(serial_number, CONFIG_DSN_LEN))
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
  if (!board_info_valid(board_id, CONFIG_PCBA_LEN))
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
	X_ARM_MMU_SECTION(0x000, 0x000, 0x10,
                    ARM_UNCACHEABLE, ARM_UNBUFFERABLE,
                    ARM_ACCESS_PERM_RW_RW); /* ROM, 16M */
	X_ARM_MMU_SECTION(0x070, 0x070, 0x010,
                    ARM_UNCACHEABLE, ARM_UNBUFFERABLE,
                    ARM_ACCESS_PERM_RW_RW); /* IRAM */
	X_ARM_MMU_SECTION(0x100, 0x100, 0x040,
                    ARM_UNCACHEABLE, ARM_UNBUFFERABLE,
                    ARM_ACCESS_PERM_RW_RW); /* SATA */
	X_ARM_MMU_SECTION(0x180, 0x180, 0x100,
                    ARM_UNCACHEABLE, ARM_UNBUFFERABLE,
                    ARM_ACCESS_PERM_RW_RW); /* IPUv3M */
	X_ARM_MMU_SECTION(0x200, 0x200, 0x200,
                    ARM_UNCACHEABLE, ARM_UNBUFFERABLE,
                    ARM_ACCESS_PERM_RW_RW); /* GPU */
	X_ARM_MMU_SECTION(0x400, 0x400, 0x300,
                    ARM_UNCACHEABLE, ARM_UNBUFFERABLE,
                    ARM_ACCESS_PERM_RW_RW); /* periperals */
	X_ARM_MMU_SECTION(0x700, 0x700, 0x400,
                    ARM_CACHEABLE, ARM_BUFFERABLE,
                    ARM_ACCESS_PERM_RW_RW); /* CSD0 1G */
	X_ARM_MMU_SECTION(0x700, 0xB00, 0x400,
                    ARM_UNCACHEABLE, ARM_UNBUFFERABLE,
                    ARM_ACCESS_PERM_RW_RW); /* CSD0 1G */
	X_ARM_MMU_SECTION(0xF00, 0xF00, 0x100,
                    ARM_UNCACHEABLE, ARM_UNBUFFERABLE,
                    ARM_ACCESS_PERM_RW_RW); /* CS1 EIM control*/
	X_ARM_MMU_SECTION(0xF80, 0xF80, 0x001,
                    ARM_UNCACHEABLE, ARM_UNBUFFERABLE,
                    ARM_ACCESS_PERM_RW_RW); /* iRam */

	/* Workaround for arm errata #709718 */
	/* Setup PRRR so device is always mapped to non-shared */
	asm volatile ("mrc p15, 0, %0, c10, c2, 0" : "=r"(i) : /*:*/);
	i &= (~(3 << 0x10));
	asm volatile ("mcr p15, 0, %0, c10, c2, 0" : : "r"(i) /*:*/);

	/* Enable MMU */
	MMU_ON();
}
#endif

static const struct board_type *get_board_type(void) 
{
  int i;

  if (!board_info_valid(board_id, CONFIG_PCBA_LEN)) {
    return NULL;
  }

  for (i = 0; i < NUM_KNOWN_BOARDS; i++) {
    if (strncmp((const char *) board_id, boards[i].id, strlen(boards[i].id)) == 0) {
	    return &(boards[i]);
    }
  }

  return NULL;
}

#ifdef CONFIG_FOR_FACTORY
static char boardid_input_buf[CONFIG_SYS_CBSIZE];
extern int readline_into_buffer (const char *const prompt, char * buffer);   
#endif

#ifdef CONFIG_IRAM_BOOT
extern void mddr_init(void);
extern void lpddr2_init(int cs);
#endif

unsigned int get_dram_size(void) 
{
  int i;
  unsigned int size = 0;

  for (i=0; i<CONFIG_NR_DRAM_BANKS; i++) {
    size += gd->bd->bi_dram[i].size;
  }

  return size;
}

int dram_init(void)
{
  const struct board_type *board;

  gd->bd->bi_dram[0].start = PHYS_SDRAM_1;

  while (1) {

    board = get_board_type();

    if (board) {

	    gd->bd->bi_dram[0].size = board->mem_size;

	    switch (board->mem_type) {
      case MEMORY_TYPE_MDDR:
#ifdef CONFIG_IRAM_BOOT
        mddr_init();
#endif
        return 0;

      case MEMORY_TYPE_LPDDR2:
#ifdef CONFIG_IRAM_BOOT
		    lpddr2_init(1);
#endif
        return 0;
	    } 
    }
    printf("Invalid board id!  Can't determine system type for RAM init.. bailing!\n");
    return 0;
  }
  return 0;
}

static void setup_uart(void)
{

#if defined(CONFIG_MX50_UART_DETECT)

  /* Use RXD pin to detect whether there is a serial connection on UART4 */
  mxc_request_iomux(MX50_PIN_UART4_RXD, IOMUX_CONFIG_GPIO);
  mxc_iomux_set_pad(MX50_PIN_UART4_RXD, 
                    PAD_CTL_DRV_HIGH | PAD_CTL_100K_PD |
                    PAD_CTL_PUE_PULL | PAD_CTL_PKE_ENABLE);

  mx50_gpio_direction(IOMUX_TO_GPIO(MX50_PIN_UART4_RXD), MX50_GPIO_DIRECTION_IN);

  if (mx50_gpio_get(IOMUX_TO_GPIO(MX50_PIN_UART4_RXD))) {
    g_uart_addr = UART4_BASE_ADDR;
  } else {
    g_uart_addr = UART1_BASE_ADDR;
  }

  mxc_free_iomux(MX50_PIN_UART4_RXD, IOMUX_CONFIG_GPIO);

#elif defined(CONFIG_MX50_UART1)
  g_uart_addr = UART1_BASE_ADDR;

#else
#error "define CONFIG_MX50_UARTx to use the mx50 UART driver"   
#endif

  if (g_uart_addr == UART1_BASE_ADDR) {
    /* UART1 RXD */
    mxc_request_iomux(MX50_PIN_UART1_RXD, IOMUX_CONFIG_ALT0);
    mxc_iomux_set_pad(MX50_PIN_UART1_RXD, 
                      PAD_CTL_DRV_HIGH | PAD_CTL_100K_PU |
                      PAD_CTL_PUE_PULL | PAD_CTL_PKE_ENABLE);
    mxc_iomux_set_input(MUX_IN_UART1_IPP_UART_RXD_MUX_SELECT_INPUT, 0x1);

    /* UART1 TXD */
    mxc_request_iomux(MX50_PIN_UART1_TXD, IOMUX_CONFIG_ALT0);
    mxc_iomux_set_pad(MX50_PIN_UART1_TXD, 
                      PAD_CTL_DRV_HIGH | PAD_CTL_100K_PU |
                      PAD_CTL_PUE_PULL | PAD_CTL_PKE_ENABLE);

  } else if (g_uart_addr == UART4_BASE_ADDR) {

    /* UART4 RXD */
    mxc_request_iomux(MX50_PIN_UART4_RXD, IOMUX_CONFIG_ALT0);
    mxc_iomux_set_pad(MX50_PIN_UART4_RXD, 
                      PAD_CTL_DRV_HIGH | PAD_CTL_100K_PU |
                      PAD_CTL_PUE_PULL | PAD_CTL_PKE_ENABLE);
    mxc_iomux_set_input(MUX_IN_UART4_IPP_UART_RXD_MUX_SELECT_INPUT, 0x1);
	
    /* UART4 TXD */
    mxc_request_iomux(MX50_PIN_UART4_TXD, IOMUX_CONFIG_ALT0);
    mxc_iomux_set_pad(MX50_PIN_UART4_TXD,
                      PAD_CTL_DRV_HIGH | PAD_CTL_100K_PU |
                      PAD_CTL_PUE_PULL | PAD_CTL_PKE_ENABLE);
  }
}

#ifdef CONFIG_MX50_I2C
void setup_i2c(unsigned int module_base)
{
	switch (module_base) {
	case I2C1_BASE_ADDR:
		/* i2c1 SDA */
		mxc_request_iomux(MX50_PIN_I2C1_SDA,
                      IOMUX_CONFIG_ALT0 | IOMUX_CONFIG_SION);
		mxc_iomux_set_pad(MX50_PIN_I2C1_SDA, PAD_CTL_SRE_FAST |
                      PAD_CTL_ODE_OPENDRAIN_ENABLE |
                      PAD_CTL_DRV_HIGH | PAD_CTL_100K_PU |
                      PAD_CTL_HYS_ENABLE);
		/* i2c1 SCL */
		mxc_request_iomux(MX50_PIN_I2C1_SCL,
                      IOMUX_CONFIG_ALT0 | IOMUX_CONFIG_SION);
		mxc_iomux_set_pad(MX50_PIN_I2C1_SCL, PAD_CTL_SRE_FAST |
                      PAD_CTL_ODE_OPENDRAIN_ENABLE |
                      PAD_CTL_DRV_HIGH | PAD_CTL_100K_PU |
                      PAD_CTL_HYS_ENABLE);
		break;
	case I2C2_BASE_ADDR:
		/* i2c2 SDA */
		mxc_request_iomux(MX50_PIN_I2C2_SDA,
                      IOMUX_CONFIG_ALT0 | IOMUX_CONFIG_SION);
		mxc_iomux_set_pad(MX50_PIN_I2C2_SDA,
                      PAD_CTL_SRE_FAST |
                      PAD_CTL_ODE_OPENDRAIN_ENABLE |
                      PAD_CTL_DRV_HIGH | PAD_CTL_100K_PU |
                      PAD_CTL_HYS_ENABLE);

		/* i2c2 SCL */
		mxc_request_iomux(MX50_PIN_I2C2_SCL,
                      IOMUX_CONFIG_ALT0 | IOMUX_CONFIG_SION);
		mxc_iomux_set_pad(MX50_PIN_I2C2_SCL,
                      PAD_CTL_SRE_FAST |
                      PAD_CTL_ODE_OPENDRAIN_ENABLE |
                      PAD_CTL_DRV_HIGH | PAD_CTL_100K_PU |
                      PAD_CTL_HYS_ENABLE);
		break;
	case I2C3_BASE_ADDR:
		/* i2c3 SDA */
		mxc_request_iomux(MX50_PIN_I2C3_SDA,
                      IOMUX_CONFIG_ALT0 | IOMUX_CONFIG_SION);
		mxc_iomux_set_pad(MX50_PIN_I2C3_SDA,
                      PAD_CTL_SRE_FAST |
                      PAD_CTL_ODE_OPENDRAIN_ENABLE |
                      PAD_CTL_DRV_HIGH | PAD_CTL_100K_PU |
                      PAD_CTL_HYS_ENABLE);

		/* i2c3 SCL */
		mxc_request_iomux(MX50_PIN_I2C3_SCL,
                      IOMUX_CONFIG_ALT0 | IOMUX_CONFIG_SION);
		mxc_iomux_set_pad(MX50_PIN_I2C3_SCL,
                      PAD_CTL_SRE_FAST |
                      PAD_CTL_ODE_OPENDRAIN_ENABLE |
                      PAD_CTL_DRV_HIGH | PAD_CTL_100K_PU |
                      PAD_CTL_HYS_ENABLE);
		break;
	default:
		printf("Invalid I2C base: 0x%x\n", module_base);
		break;
	}
}

#endif

#if defined (CONFIG_IMX_SPI)

s32 board_spi_get_cfg(struct imx_spi_dev_t *dev)
{
  switch (dev->slave.bus) {
#if defined(CONFIG_IMX_ECSPI)
  case 1:
    dev->base = CSPI1_BASE_ADDR;
    dev->freq = 1000000;
    dev->ss = 0;
    dev->ss_pol = IMX_SPI_ACTIVE_LOW;
    dev->fifo_sz = 32;
    dev->us_delay = 0;
    dev->version = IMX_SPI_VERSION_2_3;
    break;

  case 2:
    dev->base = CSPI2_BASE_ADDR;
    dev->freq = 1000000;
    dev->ss = 0;
    dev->ss_pol = IMX_SPI_ACTIVE_LOW;
    dev->fifo_sz = 32;
    dev->us_delay = 0;
    dev->version = IMX_SPI_VERSION_2_3;
    break;
#endif

#if defined(CONFIG_IMX_CSPI)
  case 3:
    dev->base = CSPI3_BASE_ADDR;
    dev->freq = 2000000;
    dev->ss = 0;
    dev->ss_pol = IMX_SPI_ACTIVE_HIGH;
    dev->fifo_sz = 32;
    dev->us_delay = 0;
    dev->version = IMX_SPI_VERSION_0_7;
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
    mxc_request_iomux(MX50_PIN_ECSPI1_MOSI, IOMUX_CONFIG_ALT0);
    mxc_iomux_set_pad(MX50_PIN_ECSPI1_MOSI, 
                      PAD_CTL_PKE_ENABLE | PAD_CTL_PUE_PULL |
                      PAD_CTL_DRV_HIGH | PAD_CTL_ODE_OPENDRAIN_NONE |
                      PAD_CTL_100K_PD);

    mxc_request_iomux(MX50_PIN_ECSPI1_MISO, IOMUX_CONFIG_ALT0);
    mxc_iomux_set_pad(MX50_PIN_ECSPI1_MISO,
                      PAD_CTL_PKE_ENABLE | PAD_CTL_PUE_PULL |
                      PAD_CTL_DRV_HIGH | PAD_CTL_ODE_OPENDRAIN_NONE |
                      PAD_CTL_100K_PD);


    mxc_request_iomux(MX50_PIN_ECSPI1_SS0, IOMUX_CONFIG_ALT0);
    mxc_iomux_set_pad(MX50_PIN_ECSPI1_SS0, 
                      PAD_CTL_PKE_ENABLE | PAD_CTL_PUE_PULL |
                      PAD_CTL_DRV_HIGH | PAD_CTL_ODE_OPENDRAIN_NONE |
                      PAD_CTL_100K_PU);
	
    mxc_request_iomux(MX50_PIN_ECSPI1_SCLK, IOMUX_CONFIG_ALT0);
    mxc_iomux_set_pad(MX50_PIN_ECSPI1_SCLK, 
                      PAD_CTL_PKE_ENABLE | PAD_CTL_PUE_PULL |
                      PAD_CTL_DRV_HIGH | PAD_CTL_ODE_OPENDRAIN_NONE |
                      PAD_CTL_100K_PD);
    break;

  case CSPI2_BASE_ADDR:
    mxc_request_iomux(MX50_PIN_ECSPI2_MOSI, IOMUX_CONFIG_ALT0);
    mxc_iomux_set_pad(MX50_PIN_ECSPI2_MOSI, 
                      PAD_CTL_PKE_ENABLE | PAD_CTL_PUE_PULL |
                      PAD_CTL_DRV_HIGH | PAD_CTL_ODE_OPENDRAIN_NONE |
                      PAD_CTL_100K_PD);

    mxc_request_iomux(MX50_PIN_ECSPI2_MISO, IOMUX_CONFIG_ALT0);
    mxc_iomux_set_pad(MX50_PIN_ECSPI2_MISO,
                      PAD_CTL_PKE_ENABLE | PAD_CTL_PUE_PULL |
                      PAD_CTL_DRV_HIGH | PAD_CTL_ODE_OPENDRAIN_NONE |
                      PAD_CTL_100K_PD);


    mxc_request_iomux(MX50_PIN_ECSPI2_SS0, IOMUX_CONFIG_ALT0);
    mxc_iomux_set_pad(MX50_PIN_ECSPI2_SS0, 
                      PAD_CTL_PKE_ENABLE | PAD_CTL_PUE_PULL |
                      PAD_CTL_DRV_HIGH | PAD_CTL_ODE_OPENDRAIN_NONE |
                      PAD_CTL_100K_PU);
	
    mxc_request_iomux(MX50_PIN_ECSPI2_SCLK, IOMUX_CONFIG_ALT0);
    mxc_iomux_set_pad(MX50_PIN_ECSPI2_SCLK, 
                      PAD_CTL_PKE_ENABLE | PAD_CTL_PUE_PULL |
                      PAD_CTL_DRV_HIGH | PAD_CTL_ODE_OPENDRAIN_NONE |
                      PAD_CTL_100K_PD);
    break;

  case CSPI3_BASE_ADDR:
    mxc_request_iomux(MX50_PIN_CSPI_MOSI, IOMUX_CONFIG_ALT0);
    mxc_iomux_set_pad(MX50_PIN_CSPI_MOSI, 
                      PAD_CTL_PKE_ENABLE | PAD_CTL_PUE_PULL |
                      PAD_CTL_DRV_HIGH | PAD_CTL_ODE_OPENDRAIN_NONE |
                      PAD_CTL_100K_PD);

    mxc_request_iomux(MX50_PIN_CSPI_MISO, IOMUX_CONFIG_ALT0);
    mxc_iomux_set_pad(MX50_PIN_CSPI_MISO,
                      PAD_CTL_PKE_ENABLE | PAD_CTL_PUE_PULL |
                      PAD_CTL_DRV_HIGH | PAD_CTL_ODE_OPENDRAIN_NONE |
                      PAD_CTL_100K_PD);


    mxc_request_iomux(MX50_PIN_CSPI_SS0, IOMUX_CONFIG_ALT0);
    mxc_iomux_set_pad(MX50_PIN_CSPI_SS0, 
                      PAD_CTL_PKE_ENABLE | PAD_CTL_PUE_PULL |
                      PAD_CTL_DRV_HIGH | PAD_CTL_ODE_OPENDRAIN_NONE |
                      PAD_CTL_100K_PU);
	
    mxc_request_iomux(MX50_PIN_CSPI_SCLK, IOMUX_CONFIG_ALT0);
    mxc_iomux_set_pad(MX50_PIN_CSPI_SCLK, 
                      PAD_CTL_PKE_ENABLE | PAD_CTL_PUE_PULL |
                      PAD_CTL_DRV_HIGH | PAD_CTL_ODE_OPENDRAIN_NONE |
                      PAD_CTL_100K_PD);
    break;

  default:
    break;
  }
}
#endif

#ifdef CONFIG_MXC_FEC
static void setup_fec(void)
{
	volatile unsigned int reg;

	/*FEC_MDIO*/
	mxc_request_iomux(MX50_PIN_SSI_RXC, IOMUX_CONFIG_ALT6);
	mxc_iomux_set_pad(MX50_PIN_SSI_RXC, 0xC);
	mxc_iomux_set_input(MUX_IN_FEC_FEC_MDI_SELECT_INPUT, 0x1);

	/*FEC_MDC*/
	mxc_request_iomux(MX50_PIN_SSI_RXFS, IOMUX_CONFIG_ALT6);
	mxc_iomux_set_pad(MX50_PIN_SSI_RXFS, 0x004);

	/* FEC RXD1 */
	mxc_request_iomux(MX50_PIN_DISP_D3, IOMUX_CONFIG_ALT2);
	mxc_iomux_set_pad(MX50_PIN_DISP_D3, 0x0);
	mxc_iomux_set_input(MUX_IN_FEC_FEC_RDATA_1_SELECT_INPUT, 0x0);

	/* FEC RXD0 */
	mxc_request_iomux(MX50_PIN_DISP_D4, IOMUX_CONFIG_ALT2);
	mxc_iomux_set_pad(MX50_PIN_DISP_D4, 0x0);
	mxc_iomux_set_input(MUX_IN_FEC_FEC_RDATA_0_SELECT_INPUT, 0x0);

  /* FEC TXD1 */
	mxc_request_iomux(MX50_PIN_DISP_D6, IOMUX_CONFIG_ALT2);
	mxc_iomux_set_pad(MX50_PIN_DISP_D6, 0x004);

	/* FEC TXD0 */
	mxc_request_iomux(MX50_PIN_DISP_D7, IOMUX_CONFIG_ALT2);
	mxc_iomux_set_pad(MX50_PIN_DISP_D7, 0x004);

	/* FEC TX_EN */
	mxc_request_iomux(MX50_PIN_DISP_D5, IOMUX_CONFIG_ALT2);
	mxc_iomux_set_pad(MX50_PIN_DISP_D5, 0x004);

	/* FEC TX_CLK */
	mxc_request_iomux(MX50_PIN_DISP_D0, IOMUX_CONFIG_ALT2);
	mxc_iomux_set_pad(MX50_PIN_DISP_D0, 0x0);
	mxc_iomux_set_input(MUX_IN_FEC_FEC_TX_CLK_SELECT_INPUT, 0x0);

	/* FEC RX_ER */
	mxc_request_iomux(MX50_PIN_DISP_D1, IOMUX_CONFIG_ALT2);
	mxc_iomux_set_pad(MX50_PIN_DISP_D1, 0x0);
	mxc_iomux_set_input(MUX_IN_FEC_FEC_RX_ER_SELECT_INPUT, 0);

	/* FEC CRS */
	mxc_request_iomux(MX50_PIN_DISP_D2, IOMUX_CONFIG_ALT2);
	mxc_iomux_set_pad(MX50_PIN_DISP_D2, 0x0);
	mxc_iomux_set_input(MUX_IN_FEC_FEC_RX_DV_SELECT_INPUT, 0);

	/* phy reset: gpio4-6 */
	mxc_request_iomux(MX50_PIN_KEY_COL3, IOMUX_CONFIG_ALT1);

	reg = readl(GPIO4_BASE_ADDR + 0x0);
	reg &= ~0x40;
	writel(reg, GPIO4_BASE_ADDR + 0x0);

	reg = readl(GPIO4_BASE_ADDR + 0x4);
	reg |= 0x40;
	writel(reg, GPIO4_BASE_ADDR + 0x4);

	udelay(500);

	reg = readl(GPIO4_BASE_ADDR + 0x0);
	reg |= 0x40;
	writel(reg, GPIO4_BASE_ADDR + 0x0);
}
#endif

#ifdef CONFIG_MMC

struct fsl_esdhc_cfg esdhc_cfg[CONFIG_SYS_FSL_ESDHC_NUM] = {
  {MMC_SDHC1_BASE_ADDR, 1, 1, 52000000},
  {MMC_SDHC2_BASE_ADDR, 1, 1, 52000000},
  /* BEN - FSL errata states that max MMC clock is 40 MHz for ESDHC3 in SDR mode */
  {MMC_SDHC3_BASE_ADDR, 1, 1, 40000000},
  {MMC_SDHC4_BASE_ADDR, 1, 1, 52000000},
};


#ifdef CONFIG_DYNAMIC_MMC_DEVNO
int get_mmc_env_devno(void)
{
	uint soc_sbmr = readl(SRC_BASE_ADDR + 0x4);
	int mmc_devno = 0;

	switch (soc_sbmr & 0x00300000) {
	default:
	case 0x0:
		mmc_devno = 0;
		break;
	case 0x00100000:
		mmc_devno = 1;
		break;
	case 0x00200000:
		mmc_devno = 2;
		break;
	}

	return mmc_devno;
}
#endif


int esdhc_gpio_init(bd_t *bis)
{
  s32 status = 0;
  u32 index = 0;
  u32 pad_cfg = 
    PAD_CTL_PKE_ENABLE | PAD_CTL_PUE_PULL | PAD_CTL_47K_PU |
    PAD_CTL_DRV_HIGH | PAD_CTL_ODE_OPENDRAIN_NONE;
		
  for (index = 0; index < CONFIG_SYS_FSL_ESDHC_NUM;
       ++index) {
    switch (index) {
	  case 0:
	    /* not used on Yoshi */
	    continue;		    
	  case 1:

	    /* Wifi SD */
#if 0 // BEN DEBUG
	    mxc_request_iomux(MX50_PIN_SD2_CMD, IOMUX_CONFIG_ALT0);
	    mxc_request_iomux(MX50_PIN_SD2_CLK, IOMUX_CONFIG_ALT0);
	    mxc_request_iomux(MX50_PIN_SD2_D0,  IOMUX_CONFIG_ALT0);
	    mxc_request_iomux(MX50_PIN_SD2_D1,  IOMUX_CONFIG_ALT0);
	    mxc_request_iomux(MX50_PIN_SD2_D2,  IOMUX_CONFIG_ALT0);
	    mxc_request_iomux(MX50_PIN_SD2_D3,  IOMUX_CONFIG_ALT0);
	    mxc_request_iomux(MX50_PIN_SD2_D4,  IOMUX_CONFIG_ALT0);
	    mxc_request_iomux(MX50_PIN_SD2_D5,  IOMUX_CONFIG_ALT0);
	    mxc_request_iomux(MX50_PIN_SD2_D6,  IOMUX_CONFIG_ALT0);
	    mxc_request_iomux(MX50_PIN_SD2_D7,  IOMUX_CONFIG_ALT0);

	    mxc_iomux_set_pad(MX50_PIN_SD2_CMD, 0x14);
	    mxc_iomux_set_pad(MX50_PIN_SD2_CLK, 0xD4);
	    mxc_iomux_set_pad(MX50_PIN_SD2_D0,  0x1D4);
	    mxc_iomux_set_pad(MX50_PIN_SD2_D1,  0x1D4);
	    mxc_iomux_set_pad(MX50_PIN_SD2_D2,  0x1D4);
	    mxc_iomux_set_pad(MX50_PIN_SD2_D3,  0x1D4);
	    mxc_iomux_set_pad(MX50_PIN_SD2_D4,  0x1D4);
	    mxc_iomux_set_pad(MX50_PIN_SD2_D5,  0x1D4);
	    mxc_iomux_set_pad(MX50_PIN_SD2_D6,  0x1D4);
	    mxc_iomux_set_pad(MX50_PIN_SD2_D7,  0x1D4);
	    break;
#else
	    continue;
#endif
	  case 2:
	    /* onboard flash */
	    mxc_request_iomux(MX50_PIN_SD3_CMD, IOMUX_CONFIG_ALT0);
	    mxc_request_iomux(MX50_PIN_SD3_CLK, IOMUX_CONFIG_ALT0);
	    mxc_request_iomux(MX50_PIN_SD3_D0,  IOMUX_CONFIG_ALT0);
	    mxc_request_iomux(MX50_PIN_SD3_D1,  IOMUX_CONFIG_ALT0);
	    mxc_request_iomux(MX50_PIN_SD3_D2,  IOMUX_CONFIG_ALT0);
	    mxc_request_iomux(MX50_PIN_SD3_D3,  IOMUX_CONFIG_ALT0);
	    mxc_request_iomux(MX50_PIN_SD3_D4,  IOMUX_CONFIG_ALT0);
	    mxc_request_iomux(MX50_PIN_SD3_D5,  IOMUX_CONFIG_ALT0);
	    mxc_request_iomux(MX50_PIN_SD3_D6,  IOMUX_CONFIG_ALT0);
	    mxc_request_iomux(MX50_PIN_SD3_D7,  IOMUX_CONFIG_ALT0);

	    mxc_iomux_set_pad(MX50_PIN_SD3_CMD, pad_cfg);
	    mxc_iomux_set_pad(MX50_PIN_SD3_CLK, 0x0);
	    mxc_iomux_set_pad(MX50_PIN_SD3_D0,  pad_cfg);
	    mxc_iomux_set_pad(MX50_PIN_SD3_D1,  pad_cfg);
	    mxc_iomux_set_pad(MX50_PIN_SD3_D2,  pad_cfg);
	    mxc_iomux_set_pad(MX50_PIN_SD3_D3,  pad_cfg);
	    mxc_iomux_set_pad(MX50_PIN_SD3_D4,  pad_cfg);
	    mxc_iomux_set_pad(MX50_PIN_SD3_D5,  pad_cfg);
	    mxc_iomux_set_pad(MX50_PIN_SD3_D6,  pad_cfg);
	    mxc_iomux_set_pad(MX50_PIN_SD3_D7,  pad_cfg);

	    /* Set pads to low voltage */
	    writel(IOMUXC_SW_PAD_CTL_LOW_VOLTAGE, IOMUXC_SW_PAD_CTL_GRP_NANDF);

	    break;

	  case 3:
	    /* SD card */
	    mxc_request_iomux(MX50_PIN_DISP_D8, IOMUX_CONFIG_ALT4);
	    mxc_request_iomux(MX50_PIN_DISP_D9, IOMUX_CONFIG_ALT4);
	    mxc_request_iomux(MX50_PIN_DISP_D10,  IOMUX_CONFIG_ALT4);
	    mxc_request_iomux(MX50_PIN_DISP_D11,  IOMUX_CONFIG_ALT4);
	    mxc_request_iomux(MX50_PIN_DISP_D12,  IOMUX_CONFIG_ALT4);
	    mxc_request_iomux(MX50_PIN_DISP_D13,  IOMUX_CONFIG_ALT4);

	    mxc_iomux_set_pad(MX50_PIN_DISP_D8, pad_cfg);
	    mxc_iomux_set_pad(MX50_PIN_DISP_D9, 0x0);
	    mxc_iomux_set_pad(MX50_PIN_DISP_D10,  pad_cfg);
	    mxc_iomux_set_pad(MX50_PIN_DISP_D11,  pad_cfg);
	    mxc_iomux_set_pad(MX50_PIN_DISP_D12,  pad_cfg);
	    mxc_iomux_set_pad(MX50_PIN_DISP_D13,  pad_cfg);

	    break;

			
	  default:
	    printf("Warning: you configured more ESDHC controller"
             "(%d) as supported by the board(2)\n",
             CONFIG_SYS_FSL_ESDHC_NUM);
	    return status;
	    break;
    }
		
    status |= fsl_esdhc_initialize(bis, &esdhc_cfg[index]);
  }

  return status;
}

int board_mmc_init(bd_t *bis)
{
	if (!esdhc_gpio_init(bis))
		return 0;
	else
		return -1;
}

#endif

int board_init(void)
{
	/* boot device */
	setup_boot_device();

	/* soc rev */
	setup_soc_rev();

	/* arch id for linux */
	gd->bd->bi_arch_number = MACH_TYPE_MX50_ARM2;

	/* boot parameters */
	gd->bd->bi_boot_params = PHYS_SDRAM_1 + 0x100;

	/* iomux for uart */
	setup_uart();

#ifdef CONFIG_MXC_FEC
	/* iomux for fec */
	setup_fec();
#endif

	return 0;
}


void board_power_off(void) 
{
#ifdef CONFIG_PMIC
  pmic_power_off();
#endif
}

void board_reset(void) 
{
#ifdef CONFIG_PMIC
  if (!pmic_set_alarm(5))
#endif
    printf("Couldn't reboot device, halting\n");

  if (is_soc_rev(CHIP_REV_1_0) == 0) {

    board_power_off();

  } else {

#ifdef CONFIG_PMIC
    pmic_reset();
#endif
  } 
}

int board_late_init(void)
{
#if defined(CONFIG_MX50_UART_DETECT)
	{
    const struct board_type *board;
	
    board = get_board_type();

    /* Finkle EVT1 boards have console on UART4 */
    if (strncmp(board->id, "00400", 5) == 0) {

	    /* reinit uboot to use UART4 */
	    if (g_uart_addr != UART4_BASE_ADDR) {
        g_uart_addr = UART4_BASE_ADDR;

        /* UART4 RXD */
        mxc_request_iomux(MX50_PIN_UART4_RXD, IOMUX_CONFIG_ALT0);
        mxc_iomux_set_pad(MX50_PIN_UART4_RXD, 
                          PAD_CTL_DRV_HIGH | PAD_CTL_100K_PU |
                          PAD_CTL_PUE_PULL | PAD_CTL_PKE_ENABLE);
        mxc_iomux_set_input(MUX_IN_UART4_IPP_UART_RXD_MUX_SELECT_INPUT, 0x1);
	
        /* UART4 TXD */
        mxc_request_iomux(MX50_PIN_UART4_TXD, IOMUX_CONFIG_ALT0);
        mxc_iomux_set_pad(MX50_PIN_UART4_TXD,
                          PAD_CTL_DRV_HIGH | PAD_CTL_100K_PU |
                          PAD_CTL_PUE_PULL | PAD_CTL_PKE_ENABLE);

        serial_init();
	    }
    }
	}
#endif

	/* drive CHRGLED_DIS low to allow PMIC to control LED state */
	mxc_request_iomux(MX50_PIN_EIM_DA2, IOMUX_CONFIG_GPIO);
	mxc_iomux_set_pad(MX50_PIN_EIM_DA2, 
                    PAD_CTL_PKE_ENABLE | PAD_CTL_PUE_PULL |
                    PAD_CTL_ODE_OPENDRAIN_NONE | PAD_CTL_DRV_HIGH| 
                    PAD_CTL_100K_PD);

	mx50_gpio_direction(IOMUX_TO_GPIO(MX50_PIN_EIM_DA2), MX50_GPIO_DIRECTION_OUT);
	mx50_gpio_set(IOMUX_TO_GPIO(MX50_PIN_EIM_DA2), 0);

#if defined(CONFIG_PMIC)
	{
    unsigned short voltage = BATTERY_INVALID_VOLTAGE;
    int ret;

    pmic_init();

    ret = pmic_adc_read_voltage(&voltage);
    if (ret) {
	    printf("Battery voltage: %d mV\n\n", voltage);
    } else {
	    printf("Battery voltage read fail!\n\n");
    }
#if defined(CONFIG_GADGET_FILE_STORAGE)
    /* stop boot if battery is too low */
    while (voltage != BATTERY_INVALID_VOLTAGE && voltage <= CONFIG_BOOT_HALT_VOLTAGE) {

      printf("Battery voltage too low.  Please plug in a charger\n");
      ret = file_storage_enable(CONFIG_BOOT_CONTINUE_VOLTAGE);
      if (ret) {
#ifdef CONFIG_CMD_HALT
		    printf("Can't enable charger.. shutting down\n");
		    board_power_off();
#endif	//CONFIG_CMD_HALT
      }

      ret = pmic_adc_read_voltage(&voltage);
      if (ret) {
		    printf("Battery voltage: %d mV\n", voltage);
      } else {
		    printf("Battery voltage read fail!\n");
      }
    }
#endif	//CONFIG_GADGET_FILE_STORAGE
	}
#endif	//CONFIG_PMIC

	/* figure out which partition to boot */
  //	check_boot_mode();

  fastboot_enable(CONFIG_MMC_BOOTFLASH, FASTBOOT_USE_DEFAULT);

	return 0;
}

int checkboard(void)
{
	const char *sn, *rev;
	const struct board_type *board;

	printf("Board: ");

	board = get_board_type();
	if (board) {
    printf("%s\n", board->name);
	} else {
    printf("Unknown\n");
	}

	printf("Boot Reason: [");

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
	case WEIM_NOR_BOOT:
		printf("NOR\n");
		break;
	case ONE_NAND_BOOT:
		printf("ONE NAND\n");
		break;
	case PATA_BOOT:
		printf("PATA\n");
		break;
	case SATA_BOOT:
		printf("SATA\n");
		break;
	case I2C_BOOT:
		printf("I2C\n");
		break;
	case SPI_NOR_BOOT:
		printf("SPI NOR\n");
		break;
	case SD_BOOT:
		printf("SD\n");
		break;
	case MMC_BOOT:
		printf("MMC\n");
		break;
	case NAND_BOOT:
		printf("NAND\n");
		break;
	case UNKNOWN_BOOT:
	default:
		printf("UNKNOWN\n");
		break;
	}

	/* serial number and board id */
	sn = (char *) get_board_serial();
	rev = (char *) get_board_id16();

	if (rev)
		printf ("Board Id: %.*s\n", CONFIG_PCBA_LEN, rev);

	if (sn)
		printf ("S/N: %.*s\n", CONFIG_DSN_LEN, sn);

	return 0;
}

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
	}

	return 0;
}

#if defined(CONFIG_HANG_FEEDBACK)
#if defined(CONFIG_PMIC)
#define led_init()    {pmic_init();}
#define led_2_off()   {pmic_enable_green_led(0);}
#define led_2_on()    {pmic_enable_green_led(1);}
#endif
#define morse_delay(t)	{{unsigned long msec=(t*100); while (msec--) { udelay(1000);}}}
#define long_gap()  {morse_delay(6);}
#define short_gap()  {morse_delay(2);}
#define gap()  {led_2_off(); morse_delay(1);}
#define dit()  {led_2_on();  morse_delay(1); gap();}
#define dah()  {led_2_on();  morse_delay(3); gap();}
void sos (void)
{
  dit(); dit(); dit(); short_gap(); /* Morse Code S */
  dah(); dah(); dah(); short_gap(); /* Morse Code O */
  dit(); dit(); dit(); short_gap(); /* Morse Code S */
  long_gap();
}
void ok (void)
{
  dah(); dah(); dah(); short_gap(); /* Morse Code O */
  dah(); dit(); dah(); short_gap(); /* Morse Code K */
  long_gap();
}
void hang_feedback (void)
{
  led_init();
  sos();
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
