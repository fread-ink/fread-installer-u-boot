/*
 * pmic_13892.c 
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
 * @file pmic_13892.c
 * @brief This file contains MC13892 specific PMIC code. This implementaion
 * may differ for each PMIC chip.
 *
 */

#include <common.h>
#include <pmic.h>
#include "pmic_13892.h"

#ifdef __DEBUG_PMIC__
#define DBG(fmt,args...)\
		serial_printf("[%s] %s:%d: "fmt,\
				__FILE__,__FUNCTION__,__LINE__, ##args)
#else
#define DBG(fmt,args...)
#endif

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

    ret = pmic_read_reg(REG_INT_SENSE0, &val);
    if (!ret)
	return ret;

    if (val & IDFACTORYS) {
	printf("Factory charging mode is ON\n");
	return 1;
    }

    return 0;
}

int pmic_init(void) 
{
    return board_pmic_init();
}

int pmic_set_chg_current(pmic_charge_type chg_type)
{
    unsigned curr = 0;
    unsigned value;

    switch (chg_type) {
      case pmic_charge_none: 
	curr = 0; 
	break;
      case pmic_charge_host: 
	curr = CHARGING_HOST; 
	break;
      case pmic_charge_usb_high: 
	curr = CHARGING_USB_HIGH; 
	break;
      case pmic_charge_third_party: 
	curr = CHARGING_THIRD_PARTY; 
	break;
      case pmic_charge_wall: 
	curr = CHARGING_WALL; 
	break;
    }

    value = curr << CHG_CURR_OFFSET;

    return pmic_wor_reg(REG_CHARGE, value, CHG_CURR_MASK);
}

int pmic_power_off(void)
{
    /*
     * This puts Atlas in USEROFF power cut mode
     */
    return pmic_wor_reg(REG_POWER_CTL0, USEROFF_SPI, USEROFF_SPI);
}

int pmic_reset(void)
{
    /* configure the PMIC for no WDIRESET */
    pmic_wor_reg(REG_POWER_CTL2, 0, WDIRESET);

    board_pmic_reset();

    return 1;
}

int pmic_charging(void)
{
    int ret;
    unsigned sense_0 = 0;

    ret = pmic_read_reg(REG_INT_SENSE0, &sense_0);
    if (!ret)
	return 0;

    DBG("snse: 0x%x\n\n", sense_0);

    return ((sense_0 & CHGDETS) != 0);
}

int pmic_charge_restart(void)
{
    int ret;
    unsigned sense_0 = 0;

    ret = pmic_read_reg(REG_INT_SENSE0, &sense_0);
    if (!ret)
	return ret;

    if (sense_0 & (CHGCURRS | CHGDETS)) {
	/* Restart the charging process */
	return pmic_wor_reg(REG_CHARGE, RESTART_CHG_STAT, RESTART_CHG_STAT);
    }
    
    return 1;
}

int pmic_set_autochg(int autochg)
{
    unsigned val = (autochg) ? 0 : AUTO_CHG_DIS;
    return pmic_wor_reg(REG_CHARGE, val, AUTO_CHG_DIS);
}

int pmic_start_charging(pmic_charge_type chg_type, int autochg)
{
    int ret;
    unsigned tmp = 0;


    /* If the factory mode is on, we should not (re)start charging */
    if (pmic_check_factory_mode())
	return 1;

    /* Battery thermistor check disable */  
    /* Allow programming of charge voltage and current */
    /* Turn off TRICKLE CHARGE */
    tmp = (BAT_TH_CHECK_DIS | VI_PROGRAM_EN);

    /* Disable auto charging to stop trickle charging at low
       battery levels.  Enable again to auto shutoff charging 
       when battery is full
    */
    ret = pmic_set_autochg(autochg);
    if (!ret)
	return ret;

    ret = pmic_wor_reg(REG_CHARGE, tmp, (BAT_TH_CHECK_DIS | RESTART_CHG_STAT | 
				   AUTO_CHG_DIS | VI_PROGRAM_EN | TRICKLE_CHG_EN));
    if (!ret)
	return ret;

    /* Set the ichrg */
    ret = pmic_set_chg_current(chg_type);
    if (!ret)
	return ret;

    /* Turn off CYCLB disable */
    /* PLIM: bits 15 and 16 */
    /* PLIMDIS: bit 17 */
    ret = pmic_wor_reg(REG_CHARGE, (0x2 << PLIM_OFFSET), (CHRGCYCLB | PLIM_MASK | PLIM_DIS));
    if (!ret)
	return ret;

    /* Start the charger state machine */
    ret = pmic_charge_restart();
    if (!ret)
	return ret;

    return ret;
}

#define CH_BATTERY_VOLTAGE		0
#define BATT_VOLTAGE_SCALE		4687	/* Scale the values from the ADC based on manual */

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
    unsigned int adc_0_reg = 0;
    unsigned int adc_1_reg = 0;
    unsigned int tmp;
    int ret, done;

    *result = 0;
    saved_result = BATTERY_INVALID_VOLTAGE;

    /* CONFIGURE ADC REG 0 */

    /* add auto inc */
    adc_0_reg |= ADC_INC;
    adc_0_reg |= ADC_CHRGRAW_D5;

    ret = pmic_wor_reg(REG_ADC0, adc_0_reg,
		 ADC_INC | ADC_BIS | ADC_CHRGRAW_D5 |
		 0xfff00ff);
    if (!ret)
	return ret;

    pmic_read_reg(REG_ADC0, &tmp);
    DBG("ADC0=0x%x\n", tmp);

    /* CONFIGURE ADC REG 1 */

    adc_1_reg |= ADC_SGL_CH;
	  
    adc_1_reg |= (CH_BATTERY_VOLTAGE << ADC_CH_0_POS) &
	ADC_CH_0_MASK;
    adc_1_reg |= (CH_BATTERY_VOLTAGE << ADC_CH_1_POS) &
	ADC_CH_1_MASK;

    adc_1_reg |= ADC_NO_ADTRIG;
    adc_1_reg |= ADC_EN;
    adc_1_reg |= ASC_ADC;

    ret = pmic_wor_reg(REG_ADC1, adc_1_reg,
		 ADC_SGL_CH | ADC_ATO | ADC_ADSEL
		 | ADC_CH_0_MASK | ADC_CH_1_MASK |
		 ADC_NO_ADTRIG | ADC_EN |
		 ADC_DELAY_MASK | ASC_ADC | ADC_BIS);
    if (!ret)
	return ret;

    ret = pmic_read_reg(REG_ADC1, &tmp);
    if (!ret)
	return ret;

    // wait for controller to clear ASC bit
    done = ~(tmp & ASC_ADC);
    while (!done) {
	udelay(1 * 100000);

	ret = pmic_read_reg(REG_ADC1, &tmp);
	if (!ret)
	    return ret;

	DBG("ADC1=0x%x\n", tmp);
	
	done = ~(tmp & ASC_ADC);
    }

    /* READ RESULT */

    ret = pmic_wor_reg(REG_ADC1, 4 << ADC_CH_1_POS,
		 ADC_CH_0_MASK | ADC_CH_1_MASK);
    if (!ret)
	return ret;
	
    ret = pmic_read_reg(REG_ADC2, &tmp);
    if (!ret)
	return ret;

    DBG("raw result=0x%x\n", tmp);

    tmp = ((tmp & ADD1_RESULT_MASK) >> 2);
    tmp = (tmp * BATT_VOLTAGE_SCALE) / 1000;

    *result = (unsigned short) tmp;
    saved_result = (unsigned short) tmp;

    DBG("scaled result=%d\n", *result);

    return ret;
}

int pmic_enable_green_led(int enable)
{
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

    return board_enable_green_led(enable);
}

int pmic_set_alarm(unsigned int secs_from_now) 
{
    int ret = 0;
    unsigned int cur_day, cur_time;
    unsigned int alarm_day, alarm_time;
    unsigned int days_from_now;

    /* Clear RTC alarm */
    ret = pmic_wor_reg(REG_INT_STATUS1, TOD, TOD);
    if (!ret)
	return ret;

    /* Clear RTC alarm interrupt mask */
    ret = pmic_wor_reg(REG_INT_MASK1, 0, TOD);
    if (!ret)
	return ret;

    /* Get the current time */
    ret = pmic_read_reg(REG_RTC_TIME, &cur_time);
    if (!ret)
	return ret;

    cur_time &= RTC_TIME_MASK;

    ret = pmic_read_reg(REG_RTC_DAY, &cur_day);
    if (!ret)
	return ret;

    cur_day &= RTC_DAY_MASK;

    alarm_time = cur_time + secs_from_now;
    days_from_now = (alarm_time / 86400);
    
    alarm_time = alarm_time % 86400;
    alarm_day = cur_day + days_from_now;

    ret = pmic_wor_reg(REG_RTC_ALARM, alarm_time, 0xFFFFFFFF);
    if (!ret)
	return ret;

    ret = pmic_wor_reg(REG_RTC_DAY_ALARM, alarm_day, 0xFFFFFFFF);
    if (!ret)
	return ret;

    return ret;
}
