/*
 * inventory.c 
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

#include <asm/arch/iomux.h>
#include <common.h>
#include <post.h>
#include "mx50_i2c.h"
#include <boardid.h>
#ifdef CONFIG_PMIC
#include <pmic.h>
#endif

#define I2C_ADDR_PROXIMITY                 0x0D // bus 0
#define I2C_ADDR_BATTERY                   0x55 // bus 1
#define I2C_ADDR_USBMUX                    0x35 // bus 1


extern const u8 *get_board_id16(void);

#if CONFIG_POST

int acclomr_addr [][4]   = { { 0x1C, 0x0D, 1 ,1 } // bus 0 Accelerometer
                           };

int papyrus_addr [][4]   = { { 0x48, 0x10, 1 ,1 } // bus 1 Papyrus
                           };

int codec_addr   [][4]   = { { 0x1A, 0x01, 2 ,2 } // bus 1 wm8962
                           };

int touch_addr   [][4]   = { { 0x50, 0x0a, 2 ,2 } // bus 2 Neonode
                           };

int check_battery (void)
{
    uchar reg = 0;
    int ret, bus;

    printf("Battery ");
    bus = i2c_get_bus_num();
    i2c_set_bus_num(1);
    ret = i2c_read(I2C_ADDR_BATTERY, 0x7E, 1, &reg, 1); //0x6c, yoshi 1.0
    if (ret)
    {
        printf("is missing\n");
    }
    else
    {
        printf("ID = 0x%x\n", reg);
    }
    i2c_set_bus_num(bus);
    return ret;
}

int check_codec (int index)
{
    unsigned int reg = 0;
    int ret, bus;

    printf("Codec ");
    bus = i2c_get_bus_num();
    i2c_set_bus_num(1);
    if (index == 0)     //wm8962
    {
        ret = i2c_read(codec_addr[index][0], codec_addr[index][1], codec_addr[index][2], (uchar*)&reg, codec_addr[index][3]);
    }
    else if(index == 1) //wm8960
    {
        ret = i2c_probe(codec_addr[index][0]);
    }
    if (ret)
    {
        printf("is missing\n");
    }
    else
    {
        printf("ID = 0x%x\n", reg);
    }
    i2c_set_bus_num(bus);
    return ret;
}

int check_usbmux (void)
{
    unsigned int reg = 0;
    int ret, bus;

    printf("USB Mux ");
    bus = i2c_get_bus_num();
    i2c_set_bus_num(1);
    ret = i2c_read(I2C_ADDR_USBMUX, 0x00, 1, (uchar*)&reg, 1); //Chip ID and Rev, AL32
    if (ret)
    {
        printf("is missing\n");
    }
    else
    {
        printf("ID = 0x%x\n", reg);
    }
    i2c_set_bus_num(bus);
    return ret;
}

int check_papyrus (int index)
{
    uchar reg = 0;
    int ret, bus;

    printf("Papyrus ");

    /* Set papyrus wakeup bit */
    mxc_request_iomux(MX50_PIN_EPDC_PWRCTRL0, IOMUX_CONFIG_GPIO);
    mxc_iomux_set_pad(MX50_PIN_EPDC_PWRCTRL0,
        PAD_CTL_PKE_NONE |
        PAD_CTL_ODE_OPENDRAIN_NONE |
        PAD_CTL_DRV_HIGH);
    mx50_gpio_direction(IOMUX_TO_GPIO(MX50_PIN_EPDC_PWRCTRL0), MX50_GPIO_DIRECTION_OUT);
    mx50_gpio_set(IOMUX_TO_GPIO(MX50_PIN_EPDC_PWRCTRL0), 1);

    udelay(2500); // wait for it to wake up

    bus = i2c_get_bus_num();
    i2c_set_bus_num(1);
    ret = i2c_read(papyrus_addr[index][0], papyrus_addr[index][1], papyrus_addr[index][2], &reg, papyrus_addr[index][3]);
    if (ret)
    {
        printf("is missing\n");
    }
    else
    {
        printf("ID = 0x%x\n", reg);
    }
    i2c_set_bus_num(bus);

    mx50_gpio_set(IOMUX_TO_GPIO(MX50_PIN_EPDC_PWRCTRL0), 0);

    return ret;
}

int check_accelerometer (int index)
{
    uchar reg = 0;
    int ret, bus;

    printf("Accelerometer ");
    bus = i2c_get_bus_num();
    i2c_set_bus_num(0);
    ret = i2c_read(acclomr_addr[index][0], acclomr_addr[index][1], acclomr_addr[index][2], &reg, acclomr_addr[index][3]);
    if (ret)
    {
        printf("is missing\n");
    }
    else
    {
        printf("ID = 0x%x\n", reg);
    }
    i2c_set_bus_num(bus);
    return ret;
}

int check_proximity (void)
{
    uchar reg = 0;
    int ret, bus;

    printf("Proximity ");
    bus = i2c_get_bus_num();
    i2c_set_bus_num(0);
    ret = i2c_read(I2C_ADDR_PROXIMITY, 0x40, 1, &reg, 1);
    if (ret)
    {
        printf("is missing\n");
    }
    else
    {
        printf("ID = 0x%x\n", reg);
    }
    i2c_set_bus_num(bus);
    return ret;
}

int check_touch (int index)
{
    uchar resp[touch_addr[index][3]];
    int i, ret, bus;

    printf("Touch ");
    memset(resp, 0, sizeof(resp));
    bus = i2c_get_bus_num();
    i2c_set_bus_num(2);
    ret = i2c_read(touch_addr[index][0], touch_addr[index][1], touch_addr[index][2], &resp[0], touch_addr[index][3]);
    if (ret)
    {
        printf("is missing\n");
    }
    else
    {
        printf("ID =");
        for (i = 0; i < sizeof(resp); i++) {
            printf(" %02x", resp[i]);
        }
        printf("\n");
    }
    i2c_set_bus_num(bus);
    return ret;
}

int check_power (void)
{
#ifdef CONFIG_PMIC
    unsigned int val = 0;
#endif
    int ret;

    printf("PMIC ");
#ifdef CONFIG_PMIC
    ret = pmic_read_reg(7, &val);
    if (ret)
    {
        printf("ID = 0x%x\n", val);
        ret = 0;
    }
    else
#endif
    {
        printf("is missing\n");
        ret = 1;
    }
    return ret;
}

int inventory_post_test (int flags)
{
    const char *rev;
    int ret   = 0;
    int index = 0; 

    /* Set i2c2 enable line */
    mxc_request_iomux(MX50_PIN_SSI_RXC, IOMUX_CONFIG_GPIO);
    mxc_iomux_set_pad(MX50_PIN_SSI_RXC,
        PAD_CTL_PKE_NONE |
        PAD_CTL_ODE_OPENDRAIN_NONE |
        PAD_CTL_DRV_MAX);
    mx50_gpio_direction(IOMUX_TO_GPIO(MX50_PIN_SSI_RXC), MX50_GPIO_DIRECTION_OUT);
    mx50_gpio_set(IOMUX_TO_GPIO(MX50_PIN_SSI_RXC), 1);

    rev = (char *) get_board_id16();
    if (BOARD_IS_YOSHI(rev))  // Only on BOARD_ID_YOSHI boards
    {
        ret  = check_battery();  // Battery may not be installed at SMT station
        ret |= check_power();
        ret |= check_papyrus(index);
        ret |= check_codec(index);
        if (BOARD_REV_GREATER(rev, BOARD_ID_YOSHI_3))  // Only on BOARD_ID_YOSHI boards after version 3.x
        {
            ret |= check_proximity();
        }
    }

    if (BOARD_IS_TEQUILA(rev))  // Only on BOARD_ID_TEQUILA boards
    {
        ret  = check_battery();  // Battery may not be installed at SMT station
        ret |= check_power();
        ret |= check_papyrus(index);

        if (BOARD_REV_GREATER(rev, BOARD_ID_TEQUILA_EVT1))  // Only on BOARD_ID_TEQUILA boards after EVT1
        {
            ret |= check_usbmux();
        }
    }

    if (BOARD_IS_WHITNEY(rev))  // Only on Whitney WAN boards
    {
        ret  = check_proximity(); // Proximity sensor is still being debated
        ret  = check_touch(index);       // Neonode are not preprogrammed
        ret  = check_battery();  // Battery may not be installed at SMT station
        ret |= check_power();
        ret |= check_papyrus(index);
        ret |= check_codec(index);
        if (BOARD_REV_GREATER(rev, BOARD_ID_WHITNEY_EVT1))  // Only on Whitney boards after EVT1
        {
            ret |= check_usbmux();
        }
    }

    if (BOARD_IS_WHITNEY_WFO(rev))  // Only on Whitney WFO boards
    {
        ret  = check_proximity();   // Proximity sensor is still being debated
        ret  = check_touch(index);       // Neonode are not preprogrammed
        ret  = check_battery();     // Battery may not be installed at SMT station
        ret |= check_power();
        ret |= check_papyrus(index);
        ret |= check_codec(index);
        if (BOARD_REV_GREATER(rev, BOARD_ID_WHITNEY_WFO_EVT1))  // Only on Whitney WFO boards after EVT1
        {
            ret |= check_usbmux();
        }
    }

    return ret;
}

#endif /* CONFIG_POST */
