/*
 * mx35_gpio.c 
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
#ifndef __ASM_ARCH_MXC_MX35_GPIO_H__
#define __ASM_ARCH_MXC_MX35_GPIO_H__

enum mx35_gpio_direction {
	MX35_GPIO_DIRECTION_IN,
	MX35_GPIO_DIRECTION_OUT,
};

int mx35_gpio_direction(unsigned int gpio,
			       enum mx35_gpio_direction direction);
void mx35_gpio_set(unsigned int gpio, unsigned int value);

#endif
