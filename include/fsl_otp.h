/*
 * fsl_otp.h 
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

#ifndef __FSL_OTP__
#define __FSL_OTP__

int fsl_otp_show(unsigned int index, unsigned int *value);
int fsl_otp_store(unsigned int index, unsigned int value);

#endif
