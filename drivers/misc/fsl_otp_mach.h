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

#ifndef __FREESCALE_OTP__
#define __FREESCALE_OTP__

static int otp_wait_busy(u32 flags);

#if defined(CONFIG_MX50) /* IMX5 below ============================= */

#include <asm/arch/mx50.h>
#include <asm/io.h>
#include "regs-ocotp-v2.h"

#define REGS_OCOTP_BASE		OCOTP_CTRL_BASE_ADDR
#define HW_OCOTP_CUSTn(n)	(0x00000030 + (n) * 0x10)
#define BF(value, field) 	(((value) << BP_##field) & BM_##field)

#define FSL_OTP_NUM_FUSES	8

#define DEF_RELAX	(15)	/* > 10.5ns */

static int set_otp_timing(void)
{
    unsigned long clk_rate = 0;
    unsigned long relax, sclk_count, rd_busy;
    u32 timing = 0;

    /* [1] get the clock. It needs the AHB clock,though doc writes APB.*/
    clk_rate = mxc_get_clock(MXC_AHB_CLK);

    /* do optimization for too many zeros */
    relax	= clk_rate / (1000000000 / DEF_RELAX) + 1;
    sclk_count = clk_rate / (1000000000 / 5000) + 1 + DEF_RELAX;
    rd_busy	= clk_rate / (1000000000 / 300)	+ 1;

    timing = BF(relax, OCOTP_TIMING_RELAX);
    timing |= BF(sclk_count, OCOTP_TIMING_SCLK_COUNT);
    timing |= BF(rd_busy, OCOTP_TIMING_RD_BUSY);

    writel(timing, REGS_OCOTP_BASE + HW_OCOTP_TIMING);
    return 0;
}

/* IMX5 does not need to open the bank anymore */
static int otp_read_prepare(void)
{
    return set_otp_timing();
}
static int otp_read_post(void)
{
    return 0;
}

static int otp_write_prepare(void)
{
    int ret = 0;

    /* [1] set timing */
    ret = set_otp_timing();
    if (ret)
	return ret;

    /* [2] wait */
    otp_wait_busy(0);
    return 0;
}

static int otp_write_post(void)
{
    /* Reload all the shadow registers */
    writel(BM_OCOTP_CTRL_RELOAD_SHADOWS,
		 REGS_OCOTP_BASE + HW_OCOTP_CTRL_SET);
    udelay(1);
    otp_wait_busy(BM_OCOTP_CTRL_RELOAD_SHADOWS);
    return 0;
}

#endif /* CONFIG_MX50 */


#endif
