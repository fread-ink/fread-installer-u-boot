/*
 * Copyright 2008-2009 Freescale Semiconductor, Inc.
 *
 * See file	CREDITS	for	list of	people who contributed to this
 * project.
 *
 * This	program	is free	software; you can redistribute it and/or
 * modify it under the terms of	the	GNU	General	Public License as
 * published by	the	Free Software Foundation; either version 2 of
 * the License,	or (at your	option)	any	later version.
 *
 * This	program	is distributed in the hope that	it will	be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A	PARTICULAR PURPOSE.	 See the
 * GNU General Public License for more details.
 *
 * You should have received	a copy of the GNU General Public License
 * along with this program;	if not,	write to the Free Software
 * Foundation, Inc., 59	Temple Place, Suite	330, Boston,
 * MA 02111-1307 USA
 */

/*!
 * @file esdhc.c
 *
 * @brief source code for the mmc card operation
 *
 * @ingroup mmc
 */

//#define DEBUG 1
#include <common.h>
#include <linux/types.h>
#include <linux/mmc/sd.h>
#include <linux/mmc/mmc.h>
#include <asm/io.h>
#include <asm/errno.h>
#include <asm/arch/sdhc.h>
#include <asm/arch/mx35_pins.h>
#include <asm/arch/mx35.h>
#include <asm/arch/iomux.h>

#ifdef CONFIG_MMC

#define RETRIES_TIMES 100

#define REG_WRITE_OR(val, reg) { \
		u32 temp = 0; \
		temp = readl(reg); \
		(temp) |= (val); \
		writel((temp), (reg)); \
		}

#define REG_WRITE_AND(val, reg) { \
		u32 temp = 0; \
		temp = readl(reg); \
		(temp) &= (val); \
		writel((temp), (reg)); \
		}

#define SDHC_DELAY_BY_100(x) { \
		u32 i; \
		for (i = 0; i < x; ++i) \
			udelay(100); \
		}


volatile u32 esdhc_base_pointer;

static void esdhc_cmd_config(esdhc_cmd_t *);
static u32 esdhc_check_response(void);
static u32 esdhc_check_data(void);
static u32 esdhc_poll_cihb_cdihb(data_present_select data_present);

static int after_reset = 0;

#ifdef DEBUG
static void esdhc_dump_regs(void) {
    printf("ESDHC_DMA_ADDRESS: 0x%x\n", readl(esdhc_base_pointer + ESDHC_DMA_ADDRESS));
    printf("ESDHC_BLOCK_SIZE: 0x%x\n", readl(esdhc_base_pointer + ESDHC_BLOCK_SZ_CNT));
    printf("ESDHC_ARGUMENT: 0x%x\n", readl(esdhc_base_pointer + ESDHC_ARGUMENT));
    printf("ESDHC_TRANSFER_MODE: 0x%x\n", readl(esdhc_base_pointer + ESDHC_TRANSFER_MODE));
    printf("ESDHC_RESPONSE0: 0x%x\n", readl(esdhc_base_pointer + ESDHC_RESPONSE));
    printf("ESDHC_PRESENT_STATE: 0x%x\n", readl(esdhc_base_pointer + ESDHC_PRESENT_STATE));
    printf("ESDHC_HOST_CONTROL: 0x%x\n", readl(esdhc_base_pointer + ESDHC_HOST_CONTROL));
    printf("ESDHC_SYSTEM_CONTROL: 0x%x\n", readl(esdhc_base_pointer + ESDHC_SYSTEM_CONTROL));
    printf("ESDHC_INT_STATUS: 0x%x\n", readl(esdhc_base_pointer + ESDHC_INT_STATUS));
    printf("ESDHC_INT_ENABLE: 0x%x\n", readl(esdhc_base_pointer + ESDHC_INT_ENABLE));
    printf("ESDHC_SIGNAL_ENABLE: 0x%x\n", readl(esdhc_base_pointer + ESDHC_SIGNAL_ENABLE));
    printf("ESDHC_ACMD12_ERR: 0x%x\n", readl(esdhc_base_pointer + ESDHC_ACMD12_ERR));
    printf("ESDHC_CAPABILITIES: 0x%x\n", readl(esdhc_base_pointer + ESDHC_CAPABILITIES));
    printf("ESDHC_WML_LEV: 0x%x\n", readl(esdhc_base_pointer + ESDHC_WML_LEV));
    printf("ESDHC_HOST_VERSION: 0x%x\n", readl(esdhc_base_pointer + ESDHC_HOST_VERSION));
}
#endif

/*!
 * Execute a software reset and set data bus width for eSDHC.
 */
u32 interface_reset()
{
	u32 reset_status = 0;
	u32 u32Retries = 0;

	debug("Entry: interface_reset");

	/* Reset the entire host controller by writing
	1 to RSTA bit of SYSCTRL Register */
	REG_WRITE_OR(ESDHC_SOFTWARE_RESET, \
			esdhc_base_pointer + ESDHC_SYSTEM_CONTROL);

	/* Wait for clearance of reset bits */
	for (u32Retries = RETRIES_TIMES; u32Retries > 0; --u32Retries) {
	    if (readl(esdhc_base_pointer + ESDHC_SYSTEM_CONTROL)	\
		    & ESDHC_SOFTWARE_RESET) {
		reset_status = 1;
	    } else {
		reset_status = 0;
		after_reset = 1;
		break;
	    }
	}

	/* Set data bus width of ESDHC */
	interface_set_bus_width(SD_BUS_WIDTH_1);

	/* Set Endianness of ESDHC */
	REG_WRITE_AND((~ESDHC_ENDIAN_MODE_MASK), \
		      esdhc_base_pointer + ESDHC_HOST_CONTROL);
	REG_WRITE_OR(ESDHC_LITTLE_ENDIAN_MODE, \
			esdhc_base_pointer + ESDHC_HOST_CONTROL);

	/* set data timeout delay to max */
	REG_WRITE_AND((~ESDHC_SYSCTL_DATA_TIMEOUT_MASK), \
		      esdhc_base_pointer + ESDHC_SYSTEM_CONTROL);
	REG_WRITE_OR(ESDHC_SYSCTL_DATA_TIMEOUT_MAX, \
			esdhc_base_pointer + ESDHC_SYSTEM_CONTROL);

	/* Set Water Mark Level register */
	writel(WRITE_READ_WATER_MARK_LEVEL, esdhc_base_pointer + ESDHC_WML_LEV);

	return reset_status;
}

/*!
 * Enable Clock and set operating frequency.
 */
void interface_configure_clock(sdhc_freq_t frequency)
{
	u32 ident_freq = 0;
	u32 oper_freq  = 0;

	/* Clear SDCLKFS, DVFS bits */
	REG_WRITE_AND((~ESDHC_SYSCTL_FREQ_MASK), \
		     esdhc_base_pointer + ESDHC_SYSTEM_CONTROL);
	ident_freq = ESDHC_SYSCTL_IDENT_FREQ_TO2;
	oper_freq  = ESDHC_SYSCTL_OPERT_FREQ_TO2;

	if (frequency == IDENTIFICATION_FREQ) {
		/* Input frequecy to eSDHC is 36 MHZ */
		/* PLL3 is the source of input frequency*/
		/*Set DTOCV and SDCLKFS bit to get SD_CLK
		of frequency below 400 KHZ (70.31 KHZ) */
		REG_WRITE_OR(ident_freq, \
			esdhc_base_pointer + ESDHC_SYSTEM_CONTROL);
	} else if (frequency == OPERATING_FREQ) {
		/*Set DTOCV and SDCLKFS bit to get SD_CLK
		of frequency around 25 MHz.(18 MHz)*/
		REG_WRITE_OR(oper_freq, \
			esdhc_base_pointer + ESDHC_SYSTEM_CONTROL);
	} else if (frequency == HIGHSPEED_FREQ) {
	    /*Set DTOCV and SDCLKFS bit to get SD_CLK
	      of frequency around 52 MHz.(36 MHz)*/
	    REG_WRITE_OR(ESDHC_SYSCTL_HS_FREQ_TO2,					\
			 esdhc_base_pointer + ESDHC_SYSTEM_CONTROL);
	}

	if (after_reset) {
	    /* Send 80 SD clock to card before first command */
	    REG_WRITE_OR(ESDHC_SYSCTL_INITA,				\
			 esdhc_base_pointer + ESDHC_SYSTEM_CONTROL);
	    after_reset = 0;
	}
}

/*!
 * Poll the CIHB & CDIHB bits of the present
 * state register and wait until it goes low.
 */
static u32 esdhc_poll_cihb_cdihb(data_present_select data_present)
{
    u32 init_status = 0;
    u32 u32Retries = 0;
    
    debug("%s: begin\n", __FUNCTION__);

    /* poll for CMD inhibit bit to be cleared */
    for (u32Retries = RETRIES_TIMES; u32Retries > 0; u32Retries--) {
	if (!(readl(esdhc_base_pointer + ESDHC_PRESENT_STATE) & \
	      ESDHC_PRESENT_STATE_CIHB)) {
	    init_status = 0;
	    break;
	}
	SDHC_DELAY_BY_100(10);
    }

    /*
     * Wait for the data line to be free (poll the CDIHB bit of
     * the present state register).
     */
    if ((0 == init_status) && data_present) {

	debug("%s: wait for data free\n", __FUNCTION__);

	for (u32Retries = RETRIES_TIMES; u32Retries > 0; u32Retries--) {
	    if (!(readl(esdhc_base_pointer + ESDHC_PRESENT_STATE) &	\
		  ESDHC_PRESENT_STATE_CDIHB)) {
		init_status =  0;
		break;
	    }

	    SDHC_DELAY_BY_100(32);
	}
    }

    debug("%s: complete (%d)\n", __FUNCTION__, init_status);

    return init_status;
}

u32 interface_set_bus_width(u32 bus_width)
{
	// clear bus width bits
	REG_WRITE_AND(~ESDHC_CTRL_BUS_WIDTH_MASK, \
		      esdhc_base_pointer + ESDHC_HOST_CONTROL);

	if (bus_width == SD_BUS_WIDTH_1) {
	    debug("%s: switch to 1 bit mode\n", __FUNCTION__);
	    return 0;
	} else if (bus_width == SD_BUS_WIDTH_4 || 
		   bus_width == MMC_BUS_WIDTH_4) {
	    REG_WRITE_OR(ESDHC_CTRL_4BITBUS, \
		      esdhc_base_pointer + ESDHC_HOST_CONTROL);
	    debug("%s: switch to 4 bit mode\n", __FUNCTION__);
	} else if (bus_width == MMC_BUS_WIDTH_8) {
	    REG_WRITE_OR(ESDHC_CTRL_8BITBUS, \
		      esdhc_base_pointer + ESDHC_HOST_CONTROL);
	    debug("%s: switch to 8 bit mode\n", __FUNCTION__);
	}

	return 0;
}

/*!
 * Execute a command and wait for the response.
 */
u32 interface_send_cmd_wait_resp(esdhc_cmd_t *cmd)
{
    int ret;
    u32 cmd_status = 0;

    /* Clear Interrupt status register */
    writel(0xFFFFFFFF, esdhc_base_pointer + ESDHC_INT_STATUS);

    /* Enable Interrupt */
    writel(ESDHC_INT_CMD_MASK, (esdhc_base_pointer + ESDHC_INT_ENABLE));

    /* wait for previous cmd to finish */
    cmd_status = esdhc_poll_cihb_cdihb(cmd->data_present);
    if (cmd_status == 1)
	return 1;

    /* Configure Command */
    esdhc_cmd_config(cmd);

    /* Check if an error occured */
    ret = esdhc_check_response();

#ifdef CONFIG_FSL_ESDHC_DMA
    if (!ret && cmd->data_present) {
	ret = esdhc_check_data();
    }  
    /* clear interrupts */
    writel(0xFFFFFFFF,			\
	   (esdhc_base_pointer + ESDHC_INT_STATUS));

    /* disable interrupts */
    writel(0, (esdhc_base_pointer + ESDHC_INT_ENABLE));  
#endif

    return ret;
}

/*!
 * Configure ESDHC registers for sending a command to MMC.
 */
static void esdhc_cmd_config(esdhc_cmd_t *cmd)
{
    u32 u32Temp = 0;
    
    /* Write Command Argument in Command Argument Register */
    writel(cmd->arg, esdhc_base_pointer + ESDHC_ARGUMENT);

#ifdef CONFIG_FSL_ESDHC_DMA
    if (cmd->data_present) {

	debug("%s: write to 0x%x\n", __FUNCTION__, cmd->dma_address);

	/* write DMA address */
	writel(cmd->dma_address, (esdhc_base_pointer + ESDHC_DMA_ADDRESS));
	
	/* enable simple DMA */
	REG_WRITE_AND(~ESDHC_CTRL_DMA_SEL_MASK, esdhc_base_pointer + ESDHC_HOST_CONTROL);
    
	REG_WRITE_OR(ESDHC_INT_DMA_MASK, (esdhc_base_pointer + ESDHC_INT_ENABLE));

	u32Temp |= ESDHC_TRNS_DMA_ENABLE;
	u32Temp |= (cmd->acmd12_enable_check) << ESDHC_AC12_ENABLE_SHIFT;
    }
#endif

    /*
     *Configure e-SDHC Register value according to Command
     */
    u32Temp |= \
	(((cmd->data_transfer)<<ESDHC_DATA_TRANSFER_SHIFT) |
	 ((cmd->response_format)<<ESDHC_RESPONSE_FORMAT_SHIFT) |
	 ((cmd->data_present)<<ESDHC_DATA_PRESENT_SHIFT) |
	 ((cmd->crc_check) << ESDHC_CRC_CHECK_SHIFT) |
	 ((cmd->cmdindex_check) << ESDHC_CMD_INDEX_CHECK_SHIFT) |
	 ((cmd->command) << ESDHC_CMD_INDEX_SHIFT) |
	 ((cmd->block_count_enable_check) << \
	  ESDHC_BLOCK_COUNT_ENABLE_SHIFT) |
	 ((cmd->multi_single_block) << \
	  ESDHC_MULTI_SINGLE_BLOCK_SELECT_SHIFT));

    /* writing CMD to transfer register starts transaction */
    writel(u32Temp, esdhc_base_pointer + ESDHC_TRANSFER_MODE);
}

/*!
 * Wait a END_CMD_RESP interrupt by interrupt status register.
 * e-SDHC sets this bit after receving command response.
 */
static u32 esdhc_check_response(void)
{
	u32 ret = 1;
	u32 status;

	/* wait for interrupt */
	while (!(status = 
		 (readl(esdhc_base_pointer + ESDHC_INT_STATUS) & ESDHC_INT_CMD_MASK))
	       );

	/* Check whether the interrupt is an END_CMD_RESP
	* or a response time out or a CRC error
	*/
	if ((status & ESDHC_INT_RESPONSE) &&
	    !(status & ESDHC_INT_TIMEOUT) &&
	    !(status & ESDHC_INT_CRC) &&
	    !(status & ESDHC_INT_INDEX) &&
	    !(status & ESDHC_INT_END_BIT)) {
		ret = 0;
	
		debug("%s: 0x%x\n", __FUNCTION__, status);
		writel(ESDHC_INT_RESPONSE,				\
	       	       (esdhc_base_pointer + ESDHC_INT_STATUS));
	} else {
	    debug("%s: fail status=0x%x\n", __FUNCTION__, status);
#ifdef DEBUG
	    esdhc_dump_regs();
#endif
	}

	return ret;
}

/*!
 * This function will read response from e-SDHC
 * register according to reponse format.
 */
void interface_read_response(esdhc_resp_t *cmd_resp)
{
	/* get response values from e-SDHC CMDRSP registers.*/
	cmd_resp->cmd_rsp0 = (u32)readl(esdhc_base_pointer + ESDHC_RESPONSE);
	cmd_resp->cmd_rsp1 = (u32)readl(esdhc_base_pointer + \
							ESDHC_RESPONSE + 4);
	cmd_resp->cmd_rsp2 = (u32)readl(esdhc_base_pointer + \
							ESDHC_RESPONSE + 8);
	cmd_resp->cmd_rsp3 = (u32)readl(esdhc_base_pointer + \
							ESDHC_RESPONSE + 12);
}

#ifndef CONFIG_FSL_ESDHC_DMA
/*!
 * Wait a BUF_READ_READY  interrupt by pooling STATUS register.
 */
static u32 esdhc_wait_buf_rdy_intr(u32 mask, u32 multi_single_block)
{
	u32 status = 0;
	u32 u32Retries = 0;

	/* Wait interrupt (BUF_READ_RDY)
	*/

	for (u32Retries = RETRIES_TIMES; u32Retries > 0; --u32Retries) {
		if (!(readl(esdhc_base_pointer + ESDHC_INT_STATUS) & mask)) {
			status = 1;
		} else {
			status = 0;
			break;
		}
		SDHC_DELAY_BY_100(10);
	}

	if (multi_single_block == MULTIPLE && \
		readl(esdhc_base_pointer + ESDHC_INT_STATUS) & mask)
		REG_WRITE_OR(mask, (esdhc_base_pointer + ESDHC_INT_STATUS));

	return status;
}

/*!
 * Wait for TC, DEBE, DCE or DTOE by polling Interrupt STATUS register.
 */
static void esdhc_wait_op_done_intr(void)
{
    while (!(readl(esdhc_base_pointer + ESDHC_INT_STATUS) &	\
	     ESDHC_INT_DATA_END))
	;
}

/*!
 * This function will read response from e-SDHC register
 * according to reponse format.
 */
u32 interface_data_read(u32 *dest_ptr, u32 transfer_size)
{
    u32 status = 1;
    u32 i = 0;
    u32 j = 0;
    u32 *tmp_ptr = dest_ptr;

    debug("Entry: interface_data_read()\n");

    /* turn off DMA */
    REG_WRITE_AND(~ESDHC_CTRL_DMA_SEL_MASK, esdhc_base_pointer + ESDHC_HOST_CONTROL);

    /* Enable Interrupt */
    writel(ESDHC_INT_DATA_MASK,	esdhc_base_pointer + ESDHC_INT_ENABLE);

    for (i = 0; i < (transfer_size) / (ESDHC_FIFO_SIZE * 4); ++i) {
	/* Wait for BRR bit to be set */
	status = esdhc_wait_buf_rdy_intr(ESDHC_INT_DATA_AVAIL,
					 SINGLE);

	debug("esdhc_wait_buf_rdy_intr: %d\n", status);

	if (!status) {
	    for (j = 0; j < ESDHC_FIFO_SIZE; ++j) {
		*tmp_ptr++ = \
		    readl(esdhc_base_pointer + ESDHC_BUFFER);
	    }

	    /* Clear the BRR */
	    writel(ESDHC_INT_DATA_AVAIL, (esdhc_base_pointer + ESDHC_INT_STATUS));
			
	} else {
	    debug("esdhc_wait_buf_rdy_intr failed\n");
	    break;
	}
    }

    esdhc_wait_op_done_intr();

    status = esdhc_check_data();

    debug("esdhc_check_data: %d\n", status);
    debug("Exit: interface_data_read()\n");

    return status;
}

/*!
 * This function will write data to device  attached to interface.
 */
u32 interface_data_write(u32 *dest_ptr, u32 blk_len)
{
    u32 status = 1;
    u32 i = 0;
    u32 j = 0;
    u32 *tmp_ptr = dest_ptr;

    debug("Entry: interface_data_write() (DMA off)\n");

    /* turn off DMA */
    REG_WRITE_AND(~ESDHC_CTRL_DMA_SEL_MASK, esdhc_base_pointer + ESDHC_HOST_CONTROL);

    /* Enable Interrupt */
    writel(ESDHC_INT_DATA_MASK, (esdhc_base_pointer + ESDHC_INT_ENABLE));

    for (i = 0; i < (blk_len) / (ESDHC_FIFO_SIZE * 4); ++i) {
	/* Wait for BWR bit to be set */
	esdhc_wait_buf_rdy_intr(ESDHC_INT_SPACE_AVAIL, \
				SINGLE);

	for (j = 0; j < ESDHC_FIFO_SIZE; ++j) {
	    writel((*tmp_ptr), esdhc_base_pointer + ESDHC_BUFFER);
	    ++tmp_ptr;
	}

	writel(ESDHC_INT_SPACE_AVAIL, (esdhc_base_pointer + ESDHC_INT_STATUS));

    }

    /* Wait for transfer complete operation interrupt */
    esdhc_wait_op_done_intr();

    /* Check for status errors */
    status = esdhc_check_data();

    debug("Exit: interface_data_write()\n");

    return status;
}

#endif

/*!
 * If READ_OP_DONE occured check ESDHC_STATUS_TIME_OUT_READ
 * and RD_CRC_ERR_CODE and
 * to determine if an error occured
 */
static u32 esdhc_check_data(void)
{
    u32 ret = 1;
    u32 status;

    debug("Entry: esdhc_check_data()\n");

    /* wait for interrupt */
    while (!(status = 
	     (readl(esdhc_base_pointer + ESDHC_INT_STATUS) & ESDHC_INT_DMA_MASK))
	   );

    /* Check whether the interrupt is an OP_DONE
     * or a data time out or a CRC error
     */
    if ((status & ESDHC_INT_DATA_END) &&
	!(status & ESDHC_INT_DATA_TIMEOUT) &&
	!(status & ESDHC_INT_DATA_CRC) && 
	!(status & ESDHC_INT_DATA_END_BIT) && 
	!(status & ESDHC_INT_ACMD12ERR) && 
	!(status & ESDHC_INT_DMA_ERROR)) {

	ret = 0;
    } else {
	debug("%s: fail status=0x%x\n", __FUNCTION__, status);
    }

#ifdef DEBUG
    if (ret)
	esdhc_dump_regs();
#endif

    debug("Exit: esdhc_check_data()\n");

    return ret;
}

/*!
 * Set Block length.
 */
void interface_config_block_info(u32 blk_len, u32 nob)
{
    /* Configure block Attributes register */
    writel((((nob & ESDHC_MAX_BLOCK_COUNT) << ESDHC_BLOCK_SHIFT) | 
	    (blk_len & ESDHC_MAX_BLOCK_LEN)),
	  (esdhc_base_pointer + ESDHC_BLOCK_SZ_CNT));
}

#if 0 // BEN TODO
/*!
 * Configure the CMD line PAD configuration for strong or weak pull-up.
 */
void esdhc_set_cmd_pullup(esdhc_pullup_t pull_up)
{
    u32 interface_esdhc = 0;
    u32 pad_val = 0;

    interface_esdhc = (readl(0x53ff080c)) & (0x000000C0) >> 6;

    if (pull_up == STRONG) {
	pad_val = PAD_CTL_PUE_PUD | PAD_CTL_PKE_ENABLE |
	    PAD_CTL_HYS_SCHMITZ | PAD_CTL_DRV_HIGH |
	    PAD_CTL_22K_PU | PAD_CTL_SRE_FAST;
    } else {
	pad_val = PAD_CTL_PUE_PUD | PAD_CTL_PKE_ENABLE |
	    PAD_CTL_HYS_SCHMITZ | PAD_CTL_DRV_MAX |
	    PAD_CTL_100K_PU | PAD_CTL_SRE_FAST;
    }

    switch (interface_esdhc) {
      case ESDHC1:
	mxc_iomux_set_pad(MX35_PIN_SD1_CMD, pad_val);
	break;
      case ESDHC2:
	mxc_iomux_set_pad(MX35_PIN_SD2_CMD, pad_val);
	break;
      case ESDHC3:
      default:
	break;
    }
}
#endif

#endif
