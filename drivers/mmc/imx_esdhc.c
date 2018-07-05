/*
 * (C) Copyright 2008-2010 Freescale Semiconductor, Inc.
 * Terry Lv, Jason Liu
 *
 * Copyright 2007, Freescale Semiconductor, Inc
 * Andy Fleming
 *
 * Based vaguely on the pxa mmc code:
 * (C) Copyright 2003
 * Kyle Harris, Nexus Technologies, Inc. kharris@nexus-tech.net
 *
 * See file CREDITS for list of people who contributed to this
 * project.
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; either version 2 of
 * the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston,
 * MA 02111-1307 USA
 */

//#define DEBUG
#include <config.h>
#include <common.h>
#include <command.h>
#include <mmc.h>
#include <fsl_esdhc.h>
#include <asm/io.h>


DECLARE_GLOBAL_DATA_PTR;

// BEN - don't malloc in early init
static struct mmc g_mmc[CONFIG_SYS_FSL_ESDHC_NUM];
static int g_cur_mmc = 0;

#ifdef CONFIG_SYS_FSL_ESDHC_DMA

#if defined(CONFIG_SYS_FSL_ESDHC_ADMA2)
#define PROCTL_INIT		(PROCTL_LE | PROCTL_ADMA2)
#elif defined(CONFIG_SYS_FSL_ESDHC_ADMA1)
#define PROCTL_INIT		(PROCTL_LE | PROCTL_ADMA1)
#else
#define PROCTL_INIT		(PROCTL_LE)
#endif

#define SDHCI_IRQ_EN_BITS		(IRQSTATEN_CC | IRQSTATEN_TC | \
				IRQSTATEN_DMAE | \
				IRQSTATEN_CINT | IRQSTATEN_AC12E | \
				IRQSTATEN_CTOE | IRQSTATEN_CCE | IRQSTATEN_CEBE | \
				IRQSTATEN_CIE | IRQSTATEN_DTOE | IRQSTATEN_DCE | IRQSTATEN_DEBE)
#else 

#define PROCTL_INIT		(PROCTL_LE)
#define SDHCI_IRQ_EN_BITS		(IRQSTATEN_CC | IRQSTATEN_TC | \
				IRQSTATEN_BWR | IRQSTATEN_BRR | \
				IRQSTATEN_CINT | IRQSTATEN_AC12E | \
				IRQSTATEN_CTOE | IRQSTATEN_CCE | IRQSTATEN_CEBE | \
				IRQSTATEN_CIE | IRQSTATEN_DTOE | IRQSTATEN_DCE | IRQSTATEN_DEBE)
#endif

struct fsl_esdhc {
	uint	dsaddr;
	uint	blkattr;
	uint	cmdarg;
	uint	xfertyp;
	uint	cmdrsp0;
	uint	cmdrsp1;
	uint	cmdrsp2;
	uint	cmdrsp3;
	uint	datport;
	uint	prsstat;
	uint	proctl;
	uint	sysctl;
	uint	irqstat;
	uint	irqstaten;
	uint	irqsigen;
	uint	autoc12err;
	uint	hostcapblt;
	uint	wml;
	char	reserved1[8];
	uint	fevt;
	uint	adma_status;
	uint	adma_addr;
	uint	reserved2;
	uint	dllctrl;
	uint	dllstatus;
	char	reserved3[148];
	uint	hostver;
	char	reserved4[780];
	uint	scr;
};

#ifdef DEBUG
static void esdhc_dump_regs(volatile struct fsl_esdhc *r) {
    printf("ESDHC_DMA_ADDRESS: 0x%x\n", readl(&r->dsaddr));
    printf("ESDHC_BLOCK_SIZE: 0x%x\n", readl(&r->blkattr));
    printf("ESDHC_ARGUMENT: 0x%x\n", readl(&r->cmdarg));
    printf("ESDHC_TRANSFER_MODE: 0x%x\n", readl(&r->xfertyp));
    printf("ESDHC_RESPONSE0: 0x%x\n", readl(&r->cmdrsp0));
    printf("ESDHC_PRESENT_STATE: 0x%x\n", readl(&r->prsstat));
    printf("ESDHC_HOST_CONTROL: 0x%x\n", readl(&r->proctl));
    printf("ESDHC_SYSTEM_CONTROL: 0x%x\n", readl(&r->sysctl));
    printf("ESDHC_INT_STATUS: 0x%x\n", readl(&r->irqstat));
    printf("ESDHC_INT_ENABLE: 0x%x\n", readl(&r->irqstaten));
    printf("ESDHC_SIGNAL_ENABLE: 0x%x\n", readl(&r->irqsigen));
    printf("ESDHC_ACMD12_ERR: 0x%x\n", readl(&r->autoc12err));
    printf("ESDHC_CAPABILITIES: 0x%x\n", readl(&r->hostcapblt));
    printf("ESDHC_WML_LEV: 0x%x\n", readl(&r->wml));
    printf("ESDHC_ADMA_STATUS: 0x%x\n", readl(&r->adma_status));
    printf("ESDHC_ADMA_ADDR: 0x%x\n", readl(&r->adma_addr));
    printf("ESDHC_HOST_VERSION: 0x%x\n", readl(&r->hostver));
    printf("\n");
}
#else
#define esdhc_dump_regs(r)
#endif

static inline void mdelay(unsigned long msec)
{
	unsigned long i;
	for (i = 0; i < msec; i++)
		udelay(1000);
}

static inline void sdelay(unsigned long sec)
{
	unsigned long i;
	for (i = 0; i < sec; i++)
		mdelay(1000);
}

/* Return the XFERTYP flags for a given command and data packet */
uint esdhc_xfertyp(struct mmc_cmd *cmd, struct mmc_data *data)
{
    uint xfertyp = 0;

    if (data) {
	xfertyp |= XFERTYP_DPSEL;
#ifdef CONFIG_SYS_FSL_ESDHC_DMA
	xfertyp |= XFERTYP_DMAEN;
#endif

	if (data->blocks > 1) {
	    xfertyp |= XFERTYP_MSBSEL;
	    xfertyp |= XFERTYP_BCEN;

	    if (data->flags & MMC_DATA_AUTO_STOP) {
		xfertyp |= XFERTYP_AC12EN;
	    }
	}

	if (data->flags & MMC_DATA_READ)
	    xfertyp |= XFERTYP_DTDSEL;
    }

    if (cmd->resp_type & MMC_RSP_CRC)
	xfertyp |= XFERTYP_CCCEN;
    if (cmd->resp_type & MMC_RSP_OPCODE)
	xfertyp |= XFERTYP_CICEN;
    if (cmd->resp_type & MMC_RSP_136)
	xfertyp |= XFERTYP_RSPTYP_136;
    else if (cmd->resp_type & MMC_RSP_BUSY)
	xfertyp |= XFERTYP_RSPTYP_48_BUSY;
    else if (cmd->resp_type & MMC_RSP_PRESENT)
	xfertyp |= XFERTYP_RSPTYP_48;

    return XFERTYP_CMD(cmd->cmdidx) | xfertyp;
}

#ifdef CONFIG_SYS_FSL_ESDHC_DMA
static u32 adma_des_table[2] __attribute__ ((__aligned__(32)));
#endif

static int esdhc_setup_data(struct mmc *mmc, struct mmc_data *data)
{
    uint wml_value;
    int timeout;
    u32 tmp;
    struct fsl_esdhc_cfg *cfg = (struct fsl_esdhc_cfg *)mmc->priv;
    volatile struct fsl_esdhc *regs = (struct fsl_esdhc *)cfg->esdhc_base;

    wml_value = data->blocksize / 4;

    if (wml_value > 0x80) 
	wml_value = 0x80;

    if (!(data->flags & MMC_DATA_READ)) {
	if ((readl(&regs->prsstat) & PRSSTAT_WPSPL) == 0) {
	    printf("\nThe SD card is locked. Can not write to a locked card.\n\n");
	    return TIMEOUT;
	}
	wml_value = wml_value << 16;
    }

    writel(wml_value, &regs->wml);

#ifdef CONFIG_SYS_FSL_ESDHC_DMA

#if defined(CONFIG_SYS_FSL_ESDHC_ADMA2)

    if ((data->flags & MMC_DATA_READ)) {
	adma_des_table[1] = (u32) data->dest;
    } else {
	adma_des_table[1] = (u32) data->src;
    }

    adma_des_table[0] = (data->blocks * data->blocksize) << FSL_ADMA_DES_LENGTH_OFFSET;
    adma_des_table[0] |= FSL_ADMA_DES_ATTR_TRAN;
    adma_des_table[0] |= FSL_ADMA_DES_ATTR_INT;
    adma_des_table[0] |= FSL_ADMA_DES_ATTR_END;
    adma_des_table[0] |= FSL_ADMA_DES_ATTR_VALID;

    /* Write the physical address to ADMA address reg */
    writel(adma_des_table, &regs->adma_addr);

#elif defined(CONFIG_SYS_FSL_ESDHC_ADMA1)

    adma_des_table[0] = (data->blocks * data->blocksize) << FSL_ADMA_DES_LENGTH_OFFSET;
    adma_des_table[0] |= FSL_ADMA_DES_ATTR_SET;
    adma_des_table[0] |= FSL_ADMA_DES_ATTR_VALID;

    if ((data->flags & MMC_DATA_READ)) {
	adma_des_table[1] = (u32) data->dest;
    } else {
	adma_des_table[1] = (u32) data->src;
    }

    if (adma_des_table[1] & 0xFFF) {
	printf("Error: ADMA1 address not 4k aligned (0x%x)\n", adma_des_table[1]);
	return -1;
    }

    adma_des_table[1] |= FSL_ADMA_DES_ATTR_TRAN;
    adma_des_table[1] |= FSL_ADMA_DES_ATTR_VALID;
    adma_des_table[1] |= FSL_ADMA_DES_ATTR_END;

    /* Write the physical address to ADMA address reg */
    writel(adma_des_table, &regs->adma_addr);

#else /* Simple DMA */
    if ((data->flags & MMC_DATA_READ)) {
	writel((u32) data->dest, &regs->dsaddr);
    } else {
	writel((u32) data->src, &regs->dsaddr);
    }
#endif

#if defined(DEBUG)
    {
	int i;

	printf("adma: 0x");
	for (i = 0; i < 8; i++) {
	    printf("%02x", ((u8 *) &adma_des_table)[i]);
	}
	printf("\n");
    }
#endif /* defined(DEBUG) */

#endif /* CONFIG_SYS_FSL_ESDHC_DMA */

    tmp = (data->blocks << 16) | data->blocksize;
    writel(tmp, &regs->blkattr);

    debug("wr=0x%x blkattr=0x%x\n", tmp, readl(&regs->blkattr));

    /* set data timeout to max */
    timeout = 14;
    tmp = (readl(&regs->sysctl) & (~SYSCTL_TIMEOUT_MASK)) | (timeout << 16);
    writel(tmp, &regs->sysctl);

    return 0;
}


/*
 * Sends a command out on the bus.  Takes the mmc pointer,
 * a command pointer, and an optional data pointer.
 */
static int
esdhc_send_cmd(struct mmc *mmc, struct mmc_cmd *cmd, struct mmc_data *data)
{
    uint	xfertyp;
    uint	irqstat;
    u32	tmp, sysctl_restore;
    struct fsl_esdhc_cfg *cfg = (struct fsl_esdhc_cfg *)mmc->priv;
    volatile struct fsl_esdhc *regs = (struct fsl_esdhc *)cfg->esdhc_base;

    /* Wait for the bus to be idle */
    while ((readl(&regs->prsstat) & PRSSTAT_CICHB) ||
	   (readl(&regs->prsstat) & PRSSTAT_CIDHB))
	mdelay(1);

    while (readl(&regs->prsstat) & PRSSTAT_DLA);

    writel(-1, &regs->irqstat);

    writel(SDHCI_IRQ_EN_BITS, &regs->irqstaten);

    debug("Sending CMD%d\n", cmd->cmdidx); 

    /* Set up for a data transfer if we have one */
    if (data) {
	int err;

	err = esdhc_setup_data(mmc, data);
	if(err) {
	    debug("%s: setup_data failure\n", __FUNCTION__);
	    return err;
	}
    }

    /* Figure out the transfer arguments */
    xfertyp = esdhc_xfertyp(cmd, data);

#ifdef CONFIG_EMMC_DDR_MODE
    if (mmc->bus_width & EXT_CSD_BUS_WIDTH_DDR)
	xfertyp |= XFERTYP_DDR_EN;
    else
	xfertyp &= ~XFERTYP_DDR_EN;
#endif

    sysctl_restore = readl(&regs->sysctl);

    /* Turn off auto-clock gate for commands with busy signaling and no data */
    if (!data && (cmd->resp_type & MMC_RSP_BUSY)) {
	writel(sysctl_restore | 0xF, &regs->sysctl);
    }
 
    /* Send the command */
    writel(cmd->cmdarg, &regs->cmdarg);
    writel(xfertyp, &regs->xfertyp);

    /* Wait for the command to complete */
    while (!(readl(&regs->irqstat) & IRQSTAT_CC));

    irqstat = readl(&regs->irqstat);
    writel(irqstat, &regs->irqstat);

    debug(" cmd status=0x%x\n", irqstat);

    if (irqstat & CMD_ERR) {
	printf("%s: command err (0x%x)\n", __FUNCTION__, irqstat);
        if (!data && (cmd->resp_type & MMC_RSP_BUSY))
                writel(sysctl_restore, &regs->sysctl);
	return COMM_ERR;
    }

    if (irqstat & IRQSTAT_CTOE) {
        if (!data && (cmd->resp_type & MMC_RSP_BUSY))
                writel(sysctl_restore, &regs->sysctl);
	return TIMEOUT;
    }
    
    if (!data && (cmd->resp_type & MMC_RSP_BUSY)) {
	int timeout = 250000;

	/* Poll on DATA0 line for cmd with busy signal for 25 s */ 
	while (timeout > 0 && !(readl(&regs->prsstat) & PRSSTAT_DAT0)) {
	    udelay(100);
	    timeout--;
	}

	writel(sysctl_restore, &regs->sysctl);

	if (timeout <= 0) {
	    printf("Timeout waiting for DAT0 to go high!\n");
	    return TIMEOUT;
	}
    }

    /* Copy the response to the response buffer */
    if (cmd->resp_type & MMC_RSP_136) {
	u32 cmdrsp3, cmdrsp2, cmdrsp1, cmdrsp0;

	cmdrsp3 = readl(&regs->cmdrsp3);
	cmdrsp2 = readl(&regs->cmdrsp2);
	cmdrsp1 = readl(&regs->cmdrsp1);
	cmdrsp0 = readl(&regs->cmdrsp0);
	cmd->response[0] = (cmdrsp3 << 8) | (cmdrsp2 >> 24);
	cmd->response[1] = (cmdrsp2 << 8) | (cmdrsp1 >> 24);
	cmd->response[2] = (cmdrsp1 << 8) | (cmdrsp0 >> 24);
	cmd->response[3] = (cmdrsp0 << 8);
    } else
	cmd->response[0] = readl(&regs->cmdrsp0);

    /* Wait until all of the blocks are transferred */
    if (data) {

#ifdef CONFIG_SYS_FSL_ESDHC_DMA
	/* wait for interrupt */
	while (!(tmp = 
		 (readl(&regs->irqstat) & IRQSTAT_TC))
	       );

	debug(" data status=0x%x\n", tmp);
	
	/* Check whether the interrupt is an OP_DONE
	 * or a data time out or a CRC error
	 */
	if (tmp & DATA_ERR) {
	    printf("%s: data2 transfer fail status=0x%x\n", __FUNCTION__, tmp);
	    return -1;
	}
#else
	int i = 0, j = 0;
	u32 *tmp_ptr = NULL;
	uint block_size = data->blocksize;
	uint block_cnt = data->blocks;

	writel(SDHCI_IRQ_EN_BITS, &regs->irqstaten);

	if (data->flags & MMC_DATA_READ) {
	    tmp_ptr = (u32 *)data->dest;

	    for (i = 0; i < (block_cnt); ++i) {
		while (!(readl(&regs->irqstat) & IRQSTAT_BRR)) 
		    ;

		for (j = 0; j < (block_size >> 2); ++j, ++tmp_ptr) {
		    *tmp_ptr = readl(&regs->datport);
		}

		tmp = readl(&regs->irqstat) & (IRQSTAT_BRR);
		writel(tmp, &regs->irqstat);
	    }
	} else {
	    tmp_ptr = (u32 *)data->src;

	    for (i = 0; i < (block_cnt); ++i) {
		while (!(readl(&regs->irqstat) & IRQSTAT_BWR))
		    ;

		for (j = 0; j < (block_size >> 2); ++j, ++tmp_ptr) {
		    writel(*tmp_ptr, &regs->datport);
		}

		tmp = readl(&regs->irqstat) & (IRQSTAT_BWR);
		writel(tmp, &regs->irqstat);
	    }
	}

	while (!(readl(&regs->irqstat) & IRQSTAT_TC)) ;
#endif

	/* BEN - Sandisk errata: wait 10us after end data bit for
	   every data command */
	udelay(10);
    }

    writel(-1, &regs->irqstat);
    debug("  success\n");

    return 0;
}

void set_sysctl(struct mmc *mmc, uint clock)
{
    int sdhc_clk = mxc_get_clock(MXC_ESDHC_CLK);
    int div, pre_div;
    struct fsl_esdhc_cfg *cfg = (struct fsl_esdhc_cfg *)mmc->priv;
    volatile struct fsl_esdhc *regs = (struct fsl_esdhc *)cfg->esdhc_base;
    uint clk;
    u32 tmp;

    debug("%s: begin\n", __FUNCTION__);

    if (sdhc_clk / 16 > clock) {
	for (pre_div = 2; pre_div < 256; pre_div *= 2)
	    if ((sdhc_clk / pre_div) <= (clock * 16))
		break;
    } else
	pre_div = 2;

    for (div = 1; div <= 16; div++)
	if ((sdhc_clk / (div * pre_div)) <= clock)
	    break;

    pre_div >>= 1;
    div -= 1;

    clk = (pre_div << 8) | (div << 4);

#ifndef CONFIG_IMX_ESDHC_V1
    tmp = readl(&regs->sysctl) & (~SYSCTL_SDCLKEN);
    writel(tmp, &regs->sysctl);
#endif

    debug("%s: setting clock\n", __FUNCTION__);

    tmp = (readl(&regs->sysctl) & (~SYSCTL_CLOCK_MASK)) | clk;
    writel(tmp, &regs->sysctl);

    udelay(10000);

#ifdef CONFIG_IMX_ESDHC_V1
    tmp = readl(&regs->sysctl) | SYSCTL_PEREN;
    writel(tmp, &regs->sysctl);
#else
    while (!(readl(&regs->prsstat) & PRSSTAT_SDSTB)) ;

    tmp = readl(&regs->sysctl) | (SYSCTL_SDCLKEN);
    writel(tmp, &regs->sysctl);
#endif

    debug("%s: exit\n", __FUNCTION__);

}

static void esdhc_dll_setup(struct mmc *mmc)
{
	struct fsl_esdhc_cfg *cfg = (struct fsl_esdhc_cfg *)mmc->priv;
	struct fsl_esdhc *regs = (struct fsl_esdhc *)cfg->esdhc_base;

	uint dll_control = readl(&regs->dllctrl);
	dll_control &= ~(ESDHC_DLLCTRL_SLV_OVERRIDE_VAL_MASK |
		ESDHC_DLLCTRL_SLV_OVERRIDE);
	dll_control |= ((ESDHC_DLLCTRL_SLV_OVERRIDE_VAL <<
		ESDHC_DLLCTRL_SLV_OVERRIDE_VAL_SHIFT) |
		ESDHC_DLLCTRL_SLV_OVERRIDE);

	writel(dll_control, &regs->dllctrl);

}

static void esdhc_set_ios(struct mmc *mmc)
{
    struct fsl_esdhc_cfg *cfg = (struct fsl_esdhc_cfg *)mmc->priv;
    struct fsl_esdhc *regs = (struct fsl_esdhc *)cfg->esdhc_base;
    u32 tmp;

    /* Set the clock speed */
    debug("%s: set clock to %d\n", __FUNCTION__,  mmc->clock);
    set_sysctl(mmc, mmc->clock);

    /* Set the bus width */
    tmp = readl(&regs->proctl) & (~(PROCTL_DTW_4 | PROCTL_DTW_8));
    writel(tmp, &regs->proctl);

    debug("%s: set bus_width to %d\n", __FUNCTION__, mmc->bus_width);

    if (mmc->bus_width == 4) {
	tmp = readl(&regs->proctl) | PROCTL_DTW_4;
	writel(tmp, &regs->proctl);
    } else if (mmc->bus_width == 8) {
	tmp = readl(&regs->proctl) | PROCTL_DTW_8;
	writel(tmp, &regs->proctl);
    } else if (mmc->bus_width == EMMC_MODE_4BIT_DDR) {
	tmp = readl(&regs->proctl) | PROCTL_DTW_4;
	writel(tmp, &regs->proctl);
	esdhc_dll_setup(mmc);
    } else if (mmc->bus_width == EMMC_MODE_8BIT_DDR) {
	tmp = readl(&regs->proctl) | PROCTL_DTW_8;
	writel(tmp, &regs->proctl);
	esdhc_dll_setup(mmc);
    }
}

static int esdhc_init(struct mmc *mmc)
{
    struct fsl_esdhc_cfg *cfg = (struct fsl_esdhc_cfg *)mmc->priv;
    struct fsl_esdhc *regs = (struct fsl_esdhc *)cfg->esdhc_base;
    int timeout = 1000;
    u32 tmp;

    debug("esdhc_init begin: base_addr=0x%x\n", (unsigned int) regs);

    /* Reset the eSDHC by writing 1 to RSTA bit of SYSCTRL Register */
    tmp = readl(&regs->sysctl) | SYSCTL_RSTA;
    writel(tmp, &regs->sysctl);

    debug(" wait for restart\n");

    timeout = 1000;
    while ((readl(&regs->sysctl) & SYSCTL_RSTA) && --timeout)
	mdelay(1);
    if (timeout <= 0) {
	printf("Controller failed to reset!\n");
	return -1;
    }

#ifdef CONFIG_IMX_ESDHC_V1
    tmp = readl(&regs->sysctl) | (SYSCTL_HCKEN | SYSCTL_IPGEN);
    writel(tmp, &regs->sysctl);
#endif

    // BEN DEBUG
    mdelay(10);

    /* Set the initial clock speed */
    set_sysctl(mmc, 400000);

    /* Put the PROCTL reg back to the default */
    writel(PROCTL_INIT, &regs->proctl);

/* BEN - this doesn't work
   timeout = 1000;
   while (!(readl(&regs->prsstat) & PRSSTAT_CINS) && --timeout)
   mdelay(1);
   if (timeout <= 0) {
   printf("No MMC card detected!\n");
   return NO_CARD_ERR;
   }
*/

    debug("  wait for init\n");

#ifndef CONFIG_IMX_ESDHC_V1
    tmp = readl(&regs->sysctl) | SYSCTL_INITA;
    writel(tmp, &regs->sysctl);

    while (readl(&regs->sysctl) & SYSCTL_INITA)
	mdelay(1);
#endif

    debug("%s complete\n", __FUNCTION__);

    return 0;
}

int fsl_esdhc_initialize(bd_t *bis, struct fsl_esdhc_cfg *cfg)
{
    struct fsl_esdhc *regs;
    struct mmc *mmc;
    u32 caps;

    if (!cfg)
	return -1;

    mmc = &(g_mmc[g_cur_mmc++]);
    memset(mmc, 0, sizeof(struct mmc));

    sprintf(mmc->name, "FSL_ESDHC");
    regs = (struct fsl_esdhc *)cfg->esdhc_base;
    mmc->priv = cfg;
    mmc->send_cmd = esdhc_send_cmd;
    mmc->set_ios = esdhc_set_ios;
    mmc->init = esdhc_init;

    caps = regs->hostcapblt;

    if (caps & ESDHC_HOSTCAPBLT_VS18)
	mmc->voltages |= MMC_VDD_165_195;
    if (caps & ESDHC_HOSTCAPBLT_VS30)
	mmc->voltages |= MMC_VDD_29_30 | MMC_VDD_30_31;
    if (caps & ESDHC_HOSTCAPBLT_VS33) {
	mmc->voltages |= MMC_VDD_32_33 | MMC_VDD_33_34;
    }
	
    mmc->voltages = MMC_VDD_35_36 | MMC_VDD_34_35 | MMC_VDD_33_34 |
	MMC_VDD_32_33 | MMC_VDD_31_32 | MMC_VDD_30_31 |
	MMC_VDD_29_30 | MMC_VDD_28_29 | MMC_VDD_27_28;

    mmc->host_caps = MMC_MODE_4BIT | MMC_MODE_8BIT;

    if (((readl(&regs->hostver) & ESDHC_HOSTVER_VVN_MASK)
	 >> ESDHC_HOSTVER_VVN_SHIFT) >= ESDHC_HOSTVER_DDR_SUPPORT)
	mmc->host_caps |= EMMC_MODE_4BIT_DDR | EMMC_MODE_8BIT_DDR;

    if (caps & ESDHC_HOSTCAPBLT_HSS)
	mmc->host_caps |= MMC_MODE_HS_52MHz | MMC_MODE_HS;

    mmc->f_min = 400000;
    mmc->f_max = MIN(mxc_get_clock(MXC_ESDHC_CLK), cfg->max_clk);

    mmc_register(mmc);

    return 0;
}
