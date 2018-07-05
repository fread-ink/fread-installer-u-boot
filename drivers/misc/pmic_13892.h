/*
 * pmic_13892.h 
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
 * @file pmic_13892.h
 * @brief This file contains MC13892 specific PMIC code. This implementaion
 * may differ for each PMIC chip.
 *
 */

#ifndef __PMIC_13892_H__
#define __PMIC_13892_H__

enum {
	REG_INT_STATUS0 = 0,
	REG_INT_MASK0,
	REG_INT_SENSE0,
	REG_INT_STATUS1,
	REG_INT_MASK1,
	REG_INT_SENSE1,
	REG_PU_MODE_S,
	REG_IDENTIFICATION,
	REG_UNUSED0,
	REG_ACC0,
	REG_ACC1,		/*10 */
	REG_UNUSED1,
	REG_UNUSED2,
	REG_POWER_CTL0,
	REG_POWER_CTL1,
	REG_POWER_CTL2,
	REG_REGEN_ASSIGN,
	REG_UNUSED3,
	REG_MEM_A,
	REG_MEM_B,
	REG_RTC_TIME,		/*20 */
	REG_RTC_ALARM,
	REG_RTC_DAY,
	REG_RTC_DAY_ALARM,
	REG_SW_0,
	REG_SW_1,
	REG_SW_2,
	REG_SW_3,
	REG_SW_4,
	REG_SW_5,
	REG_SETTING_0,		/*30 */
	REG_SETTING_1,
	REG_MODE_0,
	REG_MODE_1,
	REG_POWER_MISC,
	REG_UNUSED4,
	REG_UNUSED5,
	REG_UNUSED6,
	REG_UNUSED7,
	REG_UNUSED8,
	REG_UNUSED9,		/*40 */
	REG_UNUSED10,
	REG_UNUSED11,
	REG_ADC0,
	REG_ADC1,
	REG_ADC2,
	REG_ADC3,
	REG_ADC4,
	REG_CHARGE,
	REG_USB0,
	REG_USB1,		/*50 */
	REG_LED_CTL0,
	REG_LED_CTL1,
	REG_LED_CTL2,
	REG_LED_CTL3,
	REG_UNUSED12,
	REG_UNUSED13,
	REG_TRIM0,
	REG_TRIM1,
	REG_TEST0,
	REG_TEST1,		/*60 */
	REG_TEST2,
	REG_TEST3,
	REG_TEST4,
};

/* INTERRUPT SENSE 0 */
#define VBUSVALIDS		(1<<3)
#define IDFACTORYS		(1<<4)
#define CHGDETS         	(1<<6)
#define CHGCURRS		(1<<11)

/* INTERRUPT SENSE 1 */
#define TOD			(1<<1)

/* POWER UP SENSE */
#define CHRGSE1BS		(1<<9)

/* CHARGER 0 */
#define CHG_CURR_OFFSET		3
#define CHG_CURR_MASK		(0xF<<CHG_CURR_OFFSET)
#define TRICKLE_CHG_EN		(1<<7)
#define BAT_TH_CHECK_DIS	(1<<9)
#define PLIM_OFFSET		15
#define PLIM_MASK		(0x3<<PLIM_OFFSET)
#define PLIM_DIS		(1<<17)
#define CHRGLEDEN		(1<<18)
#define RESTART_CHG_STAT	(1<<20)
#define AUTO_CHG_DIS		(1<<21)
#define CHRGCYCLB		(1<<22)	/* BATTCYCL to restart charging at 95% full */
#define VI_PROGRAM_EN		(1<<23)

/* POWER CONTROL 0 */
#define USEROFF_SPI		(1<<3)

/* POWER CONTROL 2 */
#define WDIRESET		(1<<12)

/* RTC */
#define RTC_TIME_MASK		(0x1FFFF)
#define RTC_DAY_MASK		(0x7FFF)

/* SWITCHER 5 */
#define HALT_MHMODE		(1<<12)

/* REGULATOR MODE 1 */
#define VSD_EN			(1<<18)

/* LED CTL 0 */
#define CL_KEYPD		9
#define CL_KEYPD_MASK		(0x07<<CL_KEYPD)
#define DC_KEYPD		3
#define DC_KEYPD_MASK		(0x3F<<DC_KEYPD)
#define RP_KEYPD		(1<<2)
#define CL_GREEN		21
#define CL_GREEN_MASK		(0x07<<CL_GREEN)
#define DC_GREEN		15
#define DC_GREEN_MASK		(0x3F<<DC_GREEN)
#define RP_GREEN		(1<<14)

/* ADC 0 */
#define ADC_WAIT_TSI_0		0x001400

#define ADC_INC                 0x030000
#define ADC_BIS                 0x800000
#define ADC_CHRGRAW_D5          0x008000

/* ADC 1 */
#define ADC_EN                  0x000001
#define ADC_SGL_CH              0x000002
#define ADC_ADSEL               0x000008
#define ADC_CH_0_POS            5
#define ADC_CH_0_MASK           0x0000E0
#define ADC_CH_1_POS            8
#define ADC_CH_1_MASK           0x000700
#define ADC_DELAY_POS           11
#define ADC_DELAY_MASK          0x07F800
#define ADC_ATO                 0x080000
#define ASC_ADC                 0x100000
#define ADC_WAIT_TSI_1		0x300001
#define ADC_NO_ADTRIG           0x200000

/* ADC 2 - 4 */
#define ADD1_RESULT_MASK        0x00000FFC
#define ADD2_RESULT_MASK        0x00FFC000
#define ADC_TS_MASK             0x00FFCFFC

#define ADC_WCOMP               0x040000
#define ADC_WCOMP_H_POS         0
#define ADC_WCOMP_L_POS         9
#define ADC_WCOMP_H_MASK        0x00003F
#define ADC_WCOMP_L_MASK        0x007E00

#define ADC_MODE_MASK           0x00003F

#define CHARGING_HOST		0x1	/* Host charging with PHY active (80 mA)*/
#define CHARGING_USB_HIGH	0x5	/* Host charging after enumeration (480 mA)*/
#define CHARGING_THIRD_PARTY	0x5	/* Third party chargers (480 mA)*/
#define CHARGING_WALL		0xA	/* Wall charging (880 mA)*/

#endif
