/*
 * fastboot.h 
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

#ifndef __FASTBOOT_H__
#define __FASTBOOT_H__

#define FASTBOOT_USE_DEFAULT	-1

extern int fastboot_enable(int dev, int part);

#endif /* __FASTBOOT_H__ */
