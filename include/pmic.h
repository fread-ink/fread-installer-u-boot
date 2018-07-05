/*
 * pmic.h 
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

#ifndef __PMIC_H__
#define __PMIC_H__

typedef enum {
    pmic_charge_none = 0,
    pmic_charge_host,
    pmic_charge_usb_high,
    pmic_charge_third_party,
    pmic_charge_wall,
} pmic_charge_type;

#define BATTERY_INVALID_VOLTAGE			0xFFFF

int pmic_wor_reg(int reg, const unsigned int value, const unsigned int mask);
int pmic_set_alarm(unsigned int secs_from_now);
int pmic_enable_green_led(int enable);
int pmic_adc_read_last_voltage(unsigned short *result);
int pmic_adc_read_voltage(unsigned short *result);
int pmic_charging(void);
int pmic_charge_restart(void);
int pmic_set_autochg(int autochg);
int pmic_set_chg_current(pmic_charge_type chg_value);
int pmic_start_charging(pmic_charge_type chg_value, int autochg);
int pmic_power_off(void);
int pmic_reset(void);
int pmic_init(void);

extern int board_pmic_init(void);
extern int board_pmic_read_reg(int reg_num, unsigned int *reg_val);
extern int board_pmic_write_reg(int reg_num, const unsigned int reg_val);
extern int board_pmic_reset(void);
extern int board_enable_green_led(int enable);

#define pmic_read_reg board_pmic_read_reg
#define pmic_write_reg board_pmic_write_reg

#endif
