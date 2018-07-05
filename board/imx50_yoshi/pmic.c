/*
 * pmic.c 
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

/*!
 * @file pmic.c
 * @brief This file contains mx51 board-specific info to talk to the MC13892 PMIC.
 *
 */

#include <common.h>

#include <pmic.h>
#include <asm/arch/mx50_pins.h>
#include <asm/arch/iomux.h>
#include <spi.h>

#define MXC_PMIC_FRAME_MASK		0x00FFFFFF
#define MXC_PMIC_MAX_REG_NUM		0x3F
#define MXC_PMIC_REG_NUM_SHIFT		0x19
#define MXC_PMIC_WRITE_BIT_SHIFT	31

static struct spi_slave *pmic_spi = NULL;

int board_pmic_init(void) {

    pmic_spi = spi_setup_slave(CONFIG_IMX_SPI_PMIC_BUS, 
		    CONFIG_IMX_SPI_PMIC_CS,
		    1000000, SPI_CS_HIGH);
    if (pmic_spi == NULL) {
	printf("Error: failed to init spi bus!\n");
	return 0;
    }

    return 1;
}

int board_pmic_write_reg(int reg, const unsigned int val) 
{
    int ret;
    unsigned int frame = 0;

    if (pmic_spi == NULL) {
	printf("%s: Error spi bus not inited!\n", __FUNCTION__);
	return 0;
    }

    frame |= (1 << MXC_PMIC_WRITE_BIT_SHIFT);
    frame |= reg << MXC_PMIC_REG_NUM_SHIFT;
    frame |= val & MXC_PMIC_FRAME_MASK;

    ret = spi_xfer(pmic_spi, 32, &frame, NULL, SPI_XFER_BEGIN | SPI_XFER_END);
    if (ret < 0) {
	printf("%s: Error on spi_xfer\n", __FUNCTION__);
	return 0;
    }

    return 1;
}

int board_pmic_read_reg(int reg, unsigned int *val) 
{
    int ret;
    unsigned int frame = 0;
    
    *val = 0;

    if (pmic_spi == NULL) {
	printf("%s: Error spi bus not inited!\n", __FUNCTION__);
	return 0;
    }

    frame |= reg << MXC_PMIC_REG_NUM_SHIFT;

    ret = spi_xfer(pmic_spi, 32, &frame, val, SPI_XFER_BEGIN | SPI_XFER_END);
    if (ret < 0) {
	printf("%s: Error on spi_xfer\n", __FUNCTION__);
	return 0;
    }

    *val &= MXC_PMIC_FRAME_MASK;

    return 1;
}

int board_enable_green_led(int enable) 
{
#if 0	//MY TODO
    int pad_val = PAD_CTL_DRV_NORMAL | PAD_CTL_SRE_SLOW | PAD_CTL_DRV_1_8V |
	    PAD_CTL_HYS_CMOS | PAD_CTL_PKE_NONE | PAD_CTL_100K_PU;

    mx35_gpio_direction(IOMUX_TO_GPIO(MX35_PIN_CSI_D12), MX35_GPIO_DIRECTION_OUT);

    /* shasta/luigi controls led w/ gpio */
    if (enable) {
	mx35_gpio_set(IOMUX_TO_GPIO(MX35_PIN_CSI_D12), 1);
    } else {
	mx35_gpio_set(IOMUX_TO_GPIO(MX35_PIN_CSI_D12), 0);
    }

    mxc_iomux_set_pad(MX35_PIN_CSI_D12, pad_val);
#endif
    return 1;
}

int board_pmic_reset(void)
{
    /* Request watchdog pin as GPIO */
    mxc_request_iomux(MX50_PIN_WDOG, IOMUX_CONFIG_GPIO);

    /* Pad settings */
    mxc_iomux_set_pad(MX50_PIN_WDOG, PAD_CTL_HYS_NONE | PAD_CTL_PKE_NONE |
		      PAD_CTL_DRV_HIGH | PAD_CTL_ODE_OPENDRAIN_NONE);

    /* Direction output */
    mx50_gpio_direction(IOMUX_TO_GPIO(MX50_PIN_WDOG), MX50_GPIO_DIRECTION_OUT);
    
    /* Set low */
    mx50_gpio_set(IOMUX_TO_GPIO(MX50_PIN_WDOG), 0);

    return 1;
}
