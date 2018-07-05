/*
 * fsl_otp.c 
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

//#define DEBUG
#include <config.h>
#include <common.h>
#include <fsl_otp.h>

#include "fsl_otp_mach.h"

#ifdef DEBUG
static void otp_dump_regs(void) 
{
    int i;

    printf("OCOTP_CTRL: 0x%08x (0x%x)\n", readl(REGS_OCOTP_BASE + HW_OCOTP_CTRL), 
	   REGS_OCOTP_BASE + HW_OCOTP_CTRL);
    printf("OCOTP_TIMING: 0x%08x (0x%x)\n", readl(REGS_OCOTP_BASE + HW_OCOTP_TIMING), 
	   REGS_OCOTP_BASE + HW_OCOTP_TIMING);

    for (i = 0; i < FSL_OTP_NUM_FUSES; i++) {
	printf("OCOTP_CFG%d: 0x%08x (0x%x)\n", i, readl(REGS_OCOTP_BASE + HW_OCOTP_CUSTn(i)), 
	       REGS_OCOTP_BASE + HW_OCOTP_CUSTn(i));
    }
}
#endif

static int otp_wait_busy(u32 flags)
{
    int count;
    u32 c;

    for (count = 10000; count >= 0; count--) {
	c = readl(REGS_OCOTP_BASE + HW_OCOTP_CTRL);
	if (!(c & (BM_OCOTP_CTRL_BUSY | BM_OCOTP_CTRL_ERROR | flags)))
	    break;
    }
    if (count < 0)
	return -1;
    return 0;
}

int fsl_otp_show(unsigned int index, unsigned int *value)
{
    *value = 0;

    /* sanity check */
    if (index >= FSL_OTP_NUM_FUSES) {
	printf("%s: invalid fuse num (%d)\n", __FUNCTION__, index);
	return -1;
    }
    
    debug("%s: reading index: %d\n", __FUNCTION__, index);

    if (otp_read_prepare()) {
	printf("%s: read failure)\n", __FUNCTION__);
	return -1;
    }

    *value = readl(REGS_OCOTP_BASE + HW_OCOTP_CUSTn(index));
    otp_read_post();
    
    return 0;
}

static int otp_write_bits(int addr, u32 data, u32 magic)
{
    u32 c; /* for control register */

    /* init the control register */
    c = readl(REGS_OCOTP_BASE + HW_OCOTP_CTRL);
    c &= ~BM_OCOTP_CTRL_ADDR;
    c |= BF(addr, OCOTP_CTRL_ADDR);
    c |= BF(magic, OCOTP_CTRL_WR_UNLOCK);
    writel(c, REGS_OCOTP_BASE + HW_OCOTP_CTRL);

    /* init the data register */
    writel(data, REGS_OCOTP_BASE + HW_OCOTP_DATA);
    otp_wait_busy(0);

    udelay(2000); /* Write Postamble */
    return 0;
}

int fsl_otp_store(unsigned int index, unsigned int value)
{
    /* sanity check */
    if (index >= FSL_OTP_NUM_FUSES) {
	printf("%s: invalid fuse num (%d)\n", __FUNCTION__, index);
	return -1;
    }

    debug("%s: writing index: %d val=0x%08x\n", __FUNCTION__, index, value);

    if (otp_write_prepare()) {
	printf("%s: read failure)\n", __FUNCTION__);
	return -1;
    }

    otp_write_bits(index, value, 0x3e77);
    otp_write_post();

    return 0;
}
