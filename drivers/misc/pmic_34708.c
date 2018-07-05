/*
 * pmic_34708.c 
 *
 * Copyright 2011 Amazon Technologies, Inc. All Rights Reserved.
 *
 * The code contained herein is licensed under the GNU General Public
 * License. You may obtain a copy of the GNU General Public License
 * Version 2 or later at the following locations:
 *
 * http://www.opensource.org/licenses/gpl-license.html
 * http://www.gnu.org/copyleft/gpl.html
 */

/*!
 * @file pmic_34708.c
 * @brief This file contains MC34078 specific PMIC code. This implementaion
 * may differ for each PMIC chip.
 *
 */

#include <common.h>
#include <pmic.h>
#include "pmic_34708.h"

//#define __DEBUG_PMIC__
#ifdef __DEBUG_PMIC__
#define DBG(fmt,args...)\
		serial_printf("[%s] %s:%d: "fmt,\
				__FILE__,__FUNCTION__,__LINE__, ##args)

void pmic_dump_regs(void)
{
    int i;
    unsigned tmp;
    int ret;

    for (i = MC34708_REG_INT_STATUS0; i <= MC34708_REG_PWM_CTL; i++) {
	ret = pmic_read_reg(i, &tmp);
	if (ret)
	    printf("ripley reg %d = 0x%x\n", i, tmp);
	else
	    printf("ripley reg %d read error!\n", i);
    }
}

#else
#define DBG(fmt,args...)
#endif

#define CFG_CHARGING_POINT_UV	4200000
#define CFG_TOPPING_OFF_UA	50000
#define CFG_VBAT_TRKL_UV	3000000
#define CFG_LOWBAT_UV		3100000

#define LOWBATT_UV_TO_BITS(uv)		((uv - 3100000) / 100000)
#define VBAT_TRKL_UV_TO_BITS(uv)	((uv-2800000) / 100000)
#define CHRITEM_UV_TO_BITS(uv)		(((uv / 1000) - 50) / 50)
#define CHRCV_UV_TO_BITS(uv)		((uv - 3500000) / 20000)
#define CHRCC_UA_TO_BITS(ua)		((ua - 250000) / 100000)

#define MUSBCHG_500MA 0x1
#define MUSBCHG_950MA 0x3

int pmic_wor_reg(int reg, const unsigned int value, const unsigned int mask) 
{
    int ret;
    unsigned tmp;

    ret = pmic_read_reg(reg, &tmp);
    if (!ret) 
	return ret;
 
    tmp &= ~mask;
    tmp |= (value & mask);

    return pmic_write_reg(reg, tmp);
}

int pmic_check_factory_mode(void)
{
    int ret;
    unsigned val = 0;

    ret = pmic_read_reg(MC34708_REG_USB_DEVICE_TYPE, &val);
    if (!ret)
	return ret;

    if (val & USB_DEVICE_TYPE_FACTORY) {
	printf("Factory charging mode is ON\n");
	return 1;
    }

    return 0;
}

int pmic_init(void) 
{
    int ret;

    ret = board_pmic_init();
    if (!ret)
	return ret;
    
    /* auto switch on USB cable insert */
    ret = pmic_wor_reg(MC34708_REG_USB_CTL,
			       BITFVAL(SW_HOLD, 1), BITFMASK(SW_HOLD));
    if (!ret)
	return ret;


    return 0;
}

int pmic_set_chg_current(pmic_charge_type chg_type)
{
    unsigned curr;
    unsigned usbchg;
    unsigned val;
    int ret = 0;

    switch (chg_type) {

	/* Automatically handled, do nothing */
      case pmic_charge_none: 
      case pmic_charge_host: 
	return 1;

      case pmic_charge_usb_high: 
	usbchg = 0; //MUSBCHG_500MA;
	curr = 350000;
	break;

      case pmic_charge_wall: 
      case pmic_charge_third_party: 
	usbchg = 0; //MUSBCHG_950MA;
	curr = 850000;
	break;
    }

    DBG("Setting charge current: %d\n", curr);

    ret = pmic_wor_reg(MC34708_REG_USB_CTL, BITFVAL(MUSBCHRG, usbchg), BITFMASK(MUSBCHRG));
    if (!ret)
	return ret;

    val = BITFVAL(CHRCV, CHRCV_UV_TO_BITS(CFG_CHARGING_POINT_UV));
    val |= BITFVAL(CHRCC, CHRCC_UA_TO_BITS(curr));

    ret = pmic_wor_reg(MC34708_REG_BATTERY_PROFILE, val, BITFMASK(CHRCC) | BITFMASK(CHRCV));
    if (!ret)
	return ret;

    DBG("Set batt profile=0x%x\n", val);

    return 1;
}

int pmic_power_off(void)
{
    /*
     * This puts Atlas in USEROFF power cut mode
     */
    return pmic_wor_reg(MC34708_REG_POWER_CTL0, POWER_CTL0_USEROFFSPI, POWER_CTL0_USEROFFSPI);
}

int pmic_reset(void)
{
    /* configure the PMIC for no WDIRESET */
    pmic_wor_reg(MC34708_REG_POWER_CTL2, 0, POWER_CTL2_WDIRESET);

    board_pmic_reset();

    return 1;
}

int pmic_charging(void)
{
    int ret;
    unsigned sense_0 = 0;

    /* Detect if a cable is inserted */
    ret = pmic_read_reg(MC34708_REG_INT_SENSE0, &sense_0);
    if (!ret)
	return 0;

    DBG("snse: 0x%x\n\n", sense_0);

    return ((sense_0 & INT0_USBDETS) != 0);
}

int pmic_charge_restart(void)
{
    if (pmic_charging()) {

	DBG("start charging!\n");

	/* Restart the charging process if cable plugged in */
	return pmic_wor_reg(MC34708_REG_BATTERY_PROFILE, BITFVAL(CHREN, 1), BITFMASK(CHREN));
    }

    return 1;
}

int pmic_set_autochg(int autochg)
{
    /* Autocharge always enabled for now */
    return 1;
}

int pmic_start_charging(pmic_charge_type chg_value, int autochg)
{
    int ret = 0;

    /* If the factory mode is on, we should not (re)start charging */
    if (pmic_check_factory_mode())
	return 1;

    /* Charging is taken care of automatically in these cases */
    if (chg_value == pmic_charge_host || chg_value == pmic_charge_none) {
	DBG("chg not started!\n");
	return 0;
    }

    DBG("%s: begin\n", __FUNCTION__);

    /* set charger current termination threshold */
    ret = pmic_wor_reg(MC34708_REG_BATTERY_PROFILE,
			       BITFVAL(CHRITERM,
				       CHRITEM_UV_TO_BITS(CFG_TOPPING_OFF_UA)),
			       BITFMASK(CHRITERM));
    if (!ret)
	return ret;

    /* enable charger current termination */
    ret = pmic_wor_reg(MC34708_REG_BATTERY_PROFILE,
			       BITFVAL(CHRITERMEN, 1),
			       BITFMASK(CHRITERMEN));
    if (!ret)
	return ret;


    /* enable EOC buck */
    ret = pmic_wor_reg(MC34708_REG_BATTERY_PROFILE,
			       BITFVAL(EOCBUCKEN, 1), BITFMASK(EOCBUCKEN));
    if (!ret)
	return ret;

    /* disable manual switch */
    ret = pmic_wor_reg(MC34708_REG_USB_CTL,
			       BITFVAL(MSW, 0), BITFMASK(MSW));
    if (!ret)
	return ret;

    /* enable 1P5 large current */
    ret = pmic_wor_reg(MC34708_REG_CHARGER_DEBOUNCE,
			       BITFVAL(ILIM1P5, 1), BITFMASK(ILIM1P5));
    if (!ret)
	return ret;


    /* enable ISO */
    ret = pmic_wor_reg(MC34708_REG_CHARGER_DEBOUNCE,
			       BITFVAL(BATTISOEN, 1), BITFMASK(BATTISOEN));
    if (!ret)
	return ret;


    ret = pmic_wor_reg(MC34708_REG_CHARGER_SOURCE,
			       BITFVAL(AUXWEAKEN, 1), BITFMASK(AUXWEAKEN));
    if (!ret)
	return ret;


    ret = pmic_wor_reg(MC34708_REG_CHARGER_SOURCE,
			       BITFVAL(VBUSWEAKEN, 1),
			       BITFMASK(VBUSWEAKEN));
    if (!ret)
	return ret;


    ret = pmic_wor_reg(MC34708_REG_BATTERY_PROFILE,
			       BITFVAL(VBAT_TRKL,
				       VBAT_TRKL_UV_TO_BITS
				       (CFG_VBAT_TRKL_UV)),
			       BITFMASK(VBAT_TRKL));
    if (!ret)
	return ret;

    ret = pmic_wor_reg
		(MC34708_REG_BATTERY_PROFILE,
		 BITFVAL(LOWBATT,
			 LOWBATT_UV_TO_BITS(CFG_LOWBAT_UV)),
		 BITFMASK(LOWBATT));
    if (!ret)
	return ret;

    DBG("%s: init cmpl\n", __FUNCTION__);

    /* Set the charge current value */
    ret = pmic_set_chg_current(chg_value);
    if (!ret)
	return ret;

    /* Start the charger state machine */
    ret = pmic_charge_restart();
    if (!ret)
	return ret;

    DBG("%s: finish\n", __FUNCTION__);

    return 1;
}

#define BATT_VOLTAGE_SCALE		4692	/* Scale the values from the ADC based on manual */
#define ADC_READ_TIMEOUT		1000

static unsigned short saved_result = BATTERY_INVALID_VOLTAGE;

int pmic_adc_read_last_voltage(unsigned short *result) {

    /* just return the last saved voltage result */
    if (saved_result == BATTERY_INVALID_VOLTAGE) {
	*result = 0;
	return 0;
    }

    *result = saved_result;
    return 1;
}

int pmic_adc_read_voltage(unsigned short *result)
{
    int ret = 0;
    int timeout;
    unsigned int tmp;

    *result = 0;
    saved_result = BATTERY_INVALID_VOLTAGE;

    /* init adc registers */
    pmic_write_reg(MC34708_REG_ADC0, ADC_ADEN);
    pmic_write_reg(MC34708_REG_ADC1, 0);

    /* init adc registers */
    ret = pmic_write_reg(MC34708_REG_ADC2, ADC_BATTERY_VOLTAGE << ADC_ADSEL0_OFFSET);
    if (!ret)
	return ret;

    pmic_write_reg(MC34708_REG_ADC3, 0);


    ret = pmic_wor_reg(MC34708_REG_ADC0, ADC_ADSTART, ADC_ADSTART);
    if (!ret)
	return ret;

    DBG("wait adc done.\n");
    timeout = ADC_READ_TIMEOUT;

    while (timeout > 0) {

	udelay(1 * 100000);

	ret = pmic_read_reg(MC34708_REG_INT_STATUS0, &tmp);
	if (!ret)
	    return ret;

	DBG("ADC1=0x%x\n", tmp);
	
	if (tmp & INT0_ADCDONEI)
	    break;

	timeout--;
    }

    if (timeout == 0) {
	DBG("ADC read timeout!\n");
	return 0;
    }

    ret = pmic_wor_reg(MC34708_REG_ADC0, 0, ADC_ADSTART);
    if (!ret)
	return ret;

    ret = pmic_read_reg(MC34708_REG_ADC4, &tmp);
    if (!ret)
	return ret;

    DBG("raw result=0x%x\n", tmp);
    tmp = (tmp & ADC_ADRESULT0_MASK) >> ADC_ADRESULT0_OFFSET;
    tmp = (tmp * BATT_VOLTAGE_SCALE) / 1000;

    *result = saved_result = (unsigned short) tmp;

    DBG("scaled result=%d\n", *result);

    /* Turn off ADC module */
    pmic_write_reg(MC34708_REG_ADC0, 0);

    return 1;
}

int pmic_enable_green_led(int enable)
{
#if 0 /* BEN TODO */
    int ret = 0;

    if (enable == 1) {
	unsigned int ledval = 0;
	
	ledval |= (0x7 << CL_KEYPD);
	ledval |= (0x3f << DC_KEYPD);
	ret = pmic_wor_reg(REG_LED_CTL1, ledval, 
		     (CL_KEYPD_MASK | DC_KEYPD_MASK | RP_KEYPD));
	if (!ret)
	    return ret;

	/* disable yellow chg led */
	ret = pmic_wor_reg(REG_CHARGE, 0, CHRGLEDEN);
	if (!ret)
	    return ret;

    } else {
	unsigned int sense_0;

	ret = pmic_wor_reg(REG_LED_CTL1, 0, 
		     (CL_KEYPD_MASK | DC_KEYPD_MASK));
	if (!ret)
	    return ret;

	/* enable yellow chg led if we're charging */
	ret = pmic_read_reg(REG_INT_SENSE0, &sense_0);
	if (!ret)
	    return ret;
	ret = pmic_wor_reg(REG_CHARGE, (sense_0 & CHGDETS) ? CHRGLEDEN : 0, CHRGLEDEN);
	if (!ret)
	    return ret;

    }
#endif
    return board_enable_green_led(enable);
}

int pmic_set_alarm(unsigned int secs_from_now) 
{
    int ret = 0;
    unsigned int cur_day, cur_time;
    unsigned int alarm_day, alarm_time;
    unsigned int days_from_now;

    /* Clear RTC alarm */
    ret = pmic_wor_reg(MC34708_REG_INT_STATUS1, INT1_TOD, INT1_TOD);
    if (!ret)
	return ret;

    /* Clear RTC alarm interrupt mask */
    ret = pmic_wor_reg(MC34708_REG_INT_MASK1, 0, INT1_TOD);
    if (!ret)
	return ret;

    /* Get the current time */
    ret = pmic_read_reg(MC34708_REG_RTC_TIME, &cur_time);
    if (!ret)
	return ret;

    cur_time &= RTC_TIME_MASK;

    ret = pmic_read_reg(MC34708_REG_RTC_DAY, &cur_day);
    if (!ret)
	return ret;

    cur_day &= RTC_DAY_MASK;

    alarm_time = cur_time + secs_from_now;
    days_from_now = (alarm_time / 86400);
    
    alarm_time = alarm_time % 86400;
    alarm_day = cur_day + days_from_now;

    ret = pmic_wor_reg(MC34708_REG_RTC_ALARM, alarm_time, 0xFFFFFFFF);
    if (!ret)
	return ret;

    ret = pmic_wor_reg(MC34708_REG_RTC_DAY_ALARM, alarm_day, 0xFFFFFFFF);
    if (!ret)
	return ret;

    return ret;
}
