/*
 * cspi.c 
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

#include <common.h>

#ifdef CONFIG_MX50_CSPI

#include <asm/arch/mx50.h>
#include <asm/arch/mx50_pins.h>
#include <asm/arch/iomux.h>
#include "mx50_cspi.h"

#define DPRINTF(args...)
//#define DPRINTF(args...)  printf(args)

/****** mini cspi driver ******/
#define ECSPI_MAXLOOP 1000 /* max number of attempts to read/write FIFO */

/* pins to use */
#define MX50_ECSPI0_MISO 		MX50_PIN_CSPI_MISO
#define MX50_ECSPI0_MOSI 		MX50_PIN_CSPI_MOSI
#define MX50_ECSPI0_SCLK 		MX50_PIN_CSPI_SCLK
#define MX50_ECSPI0_SS0 		MX50_PIN_CSPI_SS0
//#define MX50_ECSPI0_SS1 		MX50_PIN_CSPI_SS1
//#define MX50_ECSPI0_SPI_RDY 		MX50_PIN_CSPI_SPI_RDY

#define MX50_ECSPI1_MISO 		MX50_PIN_ECSPI1_MISO
#define MX50_ECSPI1_MOSI 		MX50_PIN_ECSPI1_MOSI
#define MX50_ECSPI1_SCLK 		MX50_PIN_ECSPI1_SCLK
#define MX50_ECSPI1_SS0 		MX50_PIN_ECSPI1_SS0
//#define MX50_ECSPI1_SS1 		MX50_PIN_ECSPI1_SS1
//#define MX50_ECSPI1_SPI_RDY 		MX50_PIN_ECSPI1_SPI_RDY

#define MX50_ECSPI2_MISO 		MX50_PIN_ECSPI2_MISO
#define MX50_ECSPI2_MOSI 		MX50_PIN_ECSPI2_MOSI
#define MX50_ECSPI2_SCLK 		MX50_PIN_ECSPI2_SCLK
#define MX50_ECSPI2_SS0 		MX50_PIN_ECSPI2_SS0
//#define MX50_ECSPI2_SS1 		MX50_PIN_ECSPI2_SS1
//#define MX50_ECSPI2_SPI_RDY 		MX50_PIN_ECSPI2_SPI_RDY

/* */
#define MY_CSPI1_BASE_ADDR		CSPI3_BASE_ADDR			//cspi
#define MY_CSPI2_BASE_ADDR		CSPI1_BASE_ADDR			//ecspi1
#define MY_CSPI3_BASE_ADDR		CSPI2_BASE_ADDR			//ecspi2

#define MX50_ECSPI0_RXDATA 		(MY_CSPI1_BASE_ADDR)
#define MX50_ECSPI0_TXDATA 		(MY_CSPI1_BASE_ADDR+4)
#define MX50_ECSPI0_CONREG 		(MY_CSPI1_BASE_ADDR+8)
#define MX50_ECSPI0_INTREG 		(MY_CSPI1_BASE_ADDR+12)
#define MX50_ECSPI0_DMAREG 		(MY_CSPI1_BASE_ADDR+16)
#define MX50_ECSPI0_STATREG 		(MY_CSPI1_BASE_ADDR+20)
#define MX50_ECSPI0_PERIODREG 		(MY_CSPI1_BASE_ADDR+24)
#define MX50_ECSPI0_TESTREG 		(MY_CSPI1_BASE_ADDR+28)

#define MX50_ECSPI1_RXDATA 		(MY_CSPI2_BASE_ADDR)
#define MX50_ECSPI1_TXDATA 		(MY_CSPI2_BASE_ADDR+4)
#define MX50_ECSPI1_CONREG 		(MY_CSPI2_BASE_ADDR+8)
#define MX50_ECSPI1_INTREG 		(MY_CSPI2_BASE_ADDR+12)
#define MX50_ECSPI1_DMAREG 		(MY_CSPI2_BASE_ADDR+16)
#define MX50_ECSPI1_STATREG 		(MY_CSPI2_BASE_ADDR+20)
#define MX50_ECSPI1_PERIODREG 		(MY_CSPI2_BASE_ADDR+24)
#define MX50_ECSPI1_TESTREG 		(MY_CSPI2_BASE_ADDR+28)

#define MX50_ECSPI2_RXDATA 		(MY_CSPI3_BASE_ADDR)
#define MX50_ECSPI2_TXDATA 		(MY_CSPI3_BASE_ADDR+4)
#define MX50_ECSPI2_CONREG 		(MY_CSPI3_BASE_ADDR+8)
#define MX50_ECSPI2_INTREG 		(MY_CSPI3_BASE_ADDR+12)
#define MX50_ECSPI2_DMAREG 		(MY_CSPI3_BASE_ADDR+16)
#define MX50_ECSPI2_STATREG 		(MY_CSPI3_BASE_ADDR+20)
#define MX50_ECSPI2_PERIODREG 		(MY_CSPI3_BASE_ADDR+24)
#define MX50_ECSPI2_TESTREG 		(MY_CSPI3_BASE_ADDR+28)

/** CONREG bits **/
#define ECSPI_CONREG_BITCOUNT_32	0x01f00000	/* BITCOUNT = 32 */
#define ECSPI_CONREG_DATARATE_4		0x00000000	/* data rate = ipg/4 */
#define ECSPI_CONREG_DATARATE_16	0x00020000	/* data rate = ipg/16 */
#define ECSPI_CONREG_DATARATE_512	0x00070000	/* data rate = ipg/512 */
#define ECSPI_CONREG_SS0		0x00000000	/* chip select SS0 */
#define ECSPI_CONREG_SS1		0x00001000	/* chip select SS1 */
#define ECSPI_CONREG_SS2		0x00002000	/* chip select SS2 */
#define ECSPI_CONREG_SS3		0x00003000	/* chip select SS3 */
#define ECSPI_CONREG_DRCTL_DC		0x00000000	/* data ready don't care */
#define ECSPI_CONREG_DRCTL_EDGE		0x00000100	/* data ready edge trigger */
#define ECSPI_CONREG_DRCTL_LEVEL	0x00000200	/* data ready level trigger */
#define ECSPI_CONREG_SSPOL_HIGH		0x00000080	/* active high */
#define ECSPI_CONREG_SSCTL_NEGSS	0x00000040	/* negate SS between bursts */
#define ECSPI_CONREG_PHA_PHASE1		0x00000020	/* Phase 1 operation */
#define ECSPI_CONREG_POL_LOW		0x00000010	/* active low clock polarity */
#define ECSPI_CONREG_SMC_IMMEDIATE	0x00000008	/* start when write to TXFIFO */
#define ECSPI_CONREG_XCH		0x00000004	/* initiate exchange */
#define ECSPI_CONREG_MODE_MASTER	0x00000002	/* master mode */
#define ECSPI_CONREG_EN			0x00000001	/* enable bit */

#define ECSPI_STATREG_TC		0x00000100	/* transfer complete */
#define ECSPI_STATREG_BO		0x00000080	/* bit counter overflow */
#define ECSPI_STATREG_RO		0x00000040	/* RXFIFO overflow */
#define ECSPI_STATREG_RF		0x00000020	/* RXFIFO full */
#define ECSPI_STATREG_RH		0x00000010	/* RXFIFO half full */
#define ECSPI_STATREG_RR		0x00000008	/* RXFIFO ready */
#define ECSPI_STATREG_TF		0x00000004	/* TXFIFO full */
#define ECSPI_STATREG_TH		0x00000002	/* TXFIFO half empty */
#define ECSPI_STATREG_TE		0x00000001	/* TXFIFO empty */

unsigned long ecspi_RXDATA 	= MX50_ECSPI0_RXDATA;
unsigned long ecspi_TXDATA 	= MX50_ECSPI0_TXDATA;
unsigned long ecspi_CONREG 	= MX50_ECSPI0_CONREG;
unsigned long ecspi_INTREG 	= MX50_ECSPI0_INTREG;
unsigned long ecspi_DMAREG	= MX50_ECSPI0_DMAREG;
unsigned long ecspi_STATREG 	= MX50_ECSPI0_STATREG;
unsigned long ecspi_PERIODREG 	= MX50_ECSPI0_PERIODREG;
unsigned long ecspi_TESTREG 	= MX50_ECSPI0_TESTREG;
unsigned long ecspi_rxdata 	= MX50_ECSPI0_RXDATA;

static void ecspi_dump_regs(void) {
    DPRINTF("ECSPI_RXDATA=0x%08x\n", 	__REG(ecspi_RXDATA));
    DPRINTF("ECSPI_TXDATA=0x%08x\n", 	__REG(ecspi_TXDATA));
    DPRINTF("ECSPI_CONREG=0x%08x\n", 	__REG(ecspi_CONREG));
    DPRINTF("ECSPI_INTREG=0x%08x\n", 	__REG(ecspi_INTREG));
    DPRINTF("ECSPI_DMAREG=0x%08x\n", 	__REG(ecspi_DMAREG));
    DPRINTF("ECSPI_STATREG=0x%08x\n", 	__REG(ecspi_STATREG));
    DPRINTF("ECSPI_PERIODREG=0x%08x\n", 	__REG(ecspi_PERIODREG));
    DPRINTF("ECSPI_TESTREG=0x%08x\n", 	__REG(ecspi_TESTREG));
}

static void ecspi_delay(void) {
	int i=1000000;
	while(i--) ;
}

/* read all remaining data from rxfifo */
static void ecspi_rx_flush(void) {
	int maxloop=ECSPI_MAXLOOP;
	u32 junk;
	while(maxloop--) {
		if((__REG(ecspi_STATREG)&ECSPI_STATREG_RR)) {
			junk=__REG(ecspi_RXDATA);
			DPRINTF("cspi flushed %08x\n", junk);
		}
	}
}

/* loop until the tx is empty */
static void ecspi_tx_flush(void) {
	int maxloop=ECSPI_MAXLOOP;
	while(maxloop-- && !(__REG(ecspi_STATREG)&ECSPI_STATREG_TE)) {
		ecspi_delay();
	}
}

void ecspi_init(int port) {
if (port == 0)
{
	ecspi_RXDATA 	= MX50_ECSPI0_RXDATA;
	ecspi_TXDATA 	= MX50_ECSPI0_TXDATA;
	ecspi_CONREG 	= MX50_ECSPI0_CONREG;
	ecspi_INTREG 	= MX50_ECSPI0_INTREG;
	ecspi_DMAREG	= MX50_ECSPI0_DMAREG;
	ecspi_STATREG 	= MX50_ECSPI0_STATREG;
	ecspi_PERIODREG = MX50_ECSPI0_PERIODREG;
	ecspi_TESTREG 	= MX50_ECSPI0_TESTREG;
	ecspi_rxdata 	= MX50_ECSPI0_RXDATA;

    	mxc_request_iomux(MX50_ECSPI0_MISO, IOMUX_CONFIG_ALT0);
    	mxc_request_iomux(MX50_ECSPI0_MOSI, IOMUX_CONFIG_ALT0);
    	mxc_request_iomux(MX50_ECSPI0_SCLK, IOMUX_CONFIG_ALT0);
    	mxc_request_iomux(MX50_ECSPI0_SS0, IOMUX_CONFIG_ALT0);
//    	mxc_request_iomux(MX50_ECSPI0_SS1, IOMUX_CONFIG_ALT0);
//    	mxc_request_iomux(MX50_ECSPI0_SPI_RDY, IOMUX_CONFIG_ALT0);

    	mxc_iomux_set_pad(MX50_ECSPI0_MOSI,
		      PAD_CTL_DRV_VOT_HIGH | PAD_CTL_HYS_ENABLE |
		      PAD_CTL_PKE_ENABLE | PAD_CTL_PUE_PULL |
		      PAD_CTL_100K_PD | PAD_CTL_DRV_LOW);
    	mxc_iomux_set_pad(MX50_ECSPI0_MISO,
		      PAD_CTL_DRV_VOT_HIGH | PAD_CTL_HYS_ENABLE |
		      PAD_CTL_PKE_ENABLE | PAD_CTL_PUE_PULL |
		      PAD_CTL_100K_PD | PAD_CTL_DRV_LOW);
    	mxc_iomux_set_pad(MX50_ECSPI0_SS0,
		      PAD_CTL_DRV_VOT_HIGH | PAD_CTL_HYS_ENABLE |
		      PAD_CTL_PKE_ENABLE | PAD_CTL_PUE_PULL |
		      PAD_CTL_100K_PU | PAD_CTL_ODE_OPENDRAIN_NONE |
		      PAD_CTL_DRV_LOW);
    	mxc_iomux_set_pad(MX50_ECSPI0_SCLK,
		      PAD_CTL_DRV_VOT_HIGH | PAD_CTL_HYS_ENABLE |
		      PAD_CTL_PKE_ENABLE | PAD_CTL_PUE_PULL |
		      PAD_CTL_100K_PD | PAD_CTL_DRV_LOW);
//    	mxc_iomux_set_pad(MX50_ECSPI0_SPI_RDY,
//		      PAD_CTL_DRV_VOT_HIGH | PAD_CTL_HYS_ENABLE |
//		      PAD_CTL_PKE_ENABLE | PAD_CTL_PUE_PULL |
//		      PAD_CTL_100K_PU | PAD_CTL_DRV_LOW);
}
else if (port == 1)
{
	ecspi_RXDATA 	= MX50_ECSPI1_RXDATA;
	ecspi_TXDATA 	= MX50_ECSPI1_TXDATA;
	ecspi_CONREG 	= MX50_ECSPI1_CONREG;
	ecspi_INTREG 	= MX50_ECSPI1_INTREG;
	ecspi_DMAREG	= MX50_ECSPI1_DMAREG;
	ecspi_STATREG 	= MX50_ECSPI1_STATREG;
	ecspi_PERIODREG = MX50_ECSPI1_PERIODREG;
	ecspi_TESTREG 	= MX50_ECSPI1_TESTREG;
	ecspi_rxdata 	= MX50_ECSPI1_RXDATA;

    	mxc_request_iomux(MX50_ECSPI1_MISO, IOMUX_CONFIG_ALT0);
    	mxc_request_iomux(MX50_ECSPI1_MOSI, IOMUX_CONFIG_ALT0);
    	mxc_request_iomux(MX50_ECSPI1_SCLK, IOMUX_CONFIG_ALT0);
    	mxc_request_iomux(MX50_ECSPI1_SS0, IOMUX_CONFIG_ALT0);
//    	mxc_request_iomux(MX50_ECSPI1_SS1, IOMUX_CONFIG_ALT0);
//    	mxc_request_iomux(MX50_ECSPI1_SPI_RDY, IOMUX_CONFIG_ALT0);

    	mxc_iomux_set_pad(MX50_ECSPI1_MOSI,
		      PAD_CTL_DRV_VOT_HIGH | PAD_CTL_HYS_ENABLE |
		      PAD_CTL_PKE_ENABLE | PAD_CTL_PUE_PULL |
		      PAD_CTL_100K_PD | PAD_CTL_DRV_LOW);
    	mxc_iomux_set_pad(MX50_ECSPI1_MISO,
		      PAD_CTL_DRV_VOT_HIGH | PAD_CTL_HYS_ENABLE |
		      PAD_CTL_PKE_ENABLE | PAD_CTL_PUE_PULL |
		      PAD_CTL_100K_PD | PAD_CTL_DRV_LOW);
    	mxc_iomux_set_pad(MX50_ECSPI1_SS0,
		      PAD_CTL_DRV_VOT_HIGH | PAD_CTL_HYS_ENABLE |
		      PAD_CTL_PKE_ENABLE | PAD_CTL_PUE_PULL |
		      PAD_CTL_100K_PU | PAD_CTL_ODE_OPENDRAIN_NONE |
		      PAD_CTL_DRV_LOW);
    	mxc_iomux_set_pad(MX50_ECSPI1_SCLK,
		      PAD_CTL_DRV_VOT_HIGH | PAD_CTL_HYS_ENABLE |
		      PAD_CTL_PKE_ENABLE | PAD_CTL_PUE_PULL |
		      PAD_CTL_100K_PD | PAD_CTL_DRV_LOW);
//    	mxc_iomux_set_pad(MX50_ECSPI1_SPI_RDY,
//		      PAD_CTL_DRV_VOT_HIGH | PAD_CTL_HYS_ENABLE |
//		      PAD_CTL_PKE_ENABLE | PAD_CTL_PUE_PULL |
//		      PAD_CTL_100K_PU | PAD_CTL_DRV_LOW);
}
else if (port == 2)
{
	ecspi_RXDATA 	= MX50_ECSPI2_RXDATA;
	ecspi_TXDATA 	= MX50_ECSPI2_TXDATA;
	ecspi_CONREG 	= MX50_ECSPI2_CONREG;
	ecspi_INTREG 	= MX50_ECSPI2_INTREG;
	ecspi_DMAREG	= MX50_ECSPI2_DMAREG;
	ecspi_STATREG 	= MX50_ECSPI2_STATREG;
	ecspi_PERIODREG = MX50_ECSPI2_PERIODREG;
	ecspi_TESTREG 	= MX50_ECSPI2_TESTREG;
	ecspi_rxdata 	= MX50_ECSPI2_RXDATA;

    	mxc_request_iomux(MX50_ECSPI2_MISO, IOMUX_CONFIG_ALT0);
    	mxc_request_iomux(MX50_ECSPI2_MOSI, IOMUX_CONFIG_ALT0);
    	mxc_request_iomux(MX50_ECSPI2_SCLK, IOMUX_CONFIG_ALT0);
    	mxc_request_iomux(MX50_ECSPI2_SS0, IOMUX_CONFIG_ALT0);
//    	mxc_request_iomux(MX50_ECSPI2_SS1, IOMUX_CONFIG_ALT0);
//    	mxc_request_iomux(MX50_ECSPI2_SPI_RDY, IOMUX_CONFIG_ALT0);

    	mxc_iomux_set_pad(MX50_ECSPI2_MOSI,
		      PAD_CTL_DRV_VOT_HIGH | PAD_CTL_HYS_ENABLE |
		      PAD_CTL_PKE_ENABLE | PAD_CTL_PUE_PULL |
		      PAD_CTL_100K_PD | PAD_CTL_DRV_LOW);
    	mxc_iomux_set_pad(MX50_ECSPI2_MISO,
		      PAD_CTL_DRV_VOT_HIGH | PAD_CTL_HYS_ENABLE |
		      PAD_CTL_PKE_ENABLE | PAD_CTL_PUE_PULL |
		      PAD_CTL_100K_PD | PAD_CTL_DRV_LOW);
    	mxc_iomux_set_pad(MX50_ECSPI2_SS0,
		      PAD_CTL_DRV_VOT_HIGH | PAD_CTL_HYS_ENABLE |
		      PAD_CTL_PKE_ENABLE | PAD_CTL_PUE_PULL |
		      PAD_CTL_100K_PU | PAD_CTL_ODE_OPENDRAIN_NONE |
		      PAD_CTL_DRV_LOW);
    	mxc_iomux_set_pad(MX50_ECSPI2_SCLK,
		      PAD_CTL_DRV_VOT_HIGH | PAD_CTL_HYS_ENABLE |
		      PAD_CTL_PKE_ENABLE | PAD_CTL_PUE_PULL |
		      PAD_CTL_100K_PD | PAD_CTL_DRV_LOW);
//    	mxc_iomux_set_pad(MX50_ECSPI2_SPI_RDY,
//		      PAD_CTL_DRV_VOT_HIGH | PAD_CTL_HYS_ENABLE |
//		      PAD_CTL_PKE_ENABLE | PAD_CTL_PUE_PULL |
//		      PAD_CTL_100K_PU | PAD_CTL_DRV_LOW);
}
else
{
	printf("init fail on port [%d]\n", port);
	return;
}

    __REG(ecspi_CONREG) = ECSPI_CONREG_EN
	|ECSPI_CONREG_MODE_MASTER
	|ECSPI_CONREG_SSPOL_HIGH
	|ECSPI_CONREG_DATARATE_16
	|ECSPI_CONREG_BITCOUNT_32
	|ECSPI_CONREG_SS0;
    __REG(ecspi_INTREG) = 0; /* no interrupts */
    __REG(ecspi_DMAREG) = 0; /* no DMA */
    __REG(ecspi_PERIODREG) = 500; /* 500 waits states - SPI clock source */
    ecspi_rx_flush();
}

void ecspi_shutdown(void) {
	ecspi_tx_flush();
	__REG(ecspi_CONREG) = 0; /* disable the interface */
	/* TODO: disable iomux settings */
}

int ecspi_write(u32 d) {
	int maxloop=ECSPI_MAXLOOP;

	DPRINTF("ecspi register [%08x][%08x]\n", ecspi_CONREG, ecspi_STATREG);

	while(maxloop--) {
		/* loop though until space is available in queue */
		if((__REG(ecspi_STATREG)&ECSPI_STATREG_TF)==0) {
			__REG(ecspi_TXDATA)=d;
			__REG(ecspi_CONREG)|=ECSPI_CONREG_XCH;
			return 1; /* success */
		}
	}
	printf("cspi write failed\n");
	return 0; /* failure */
}

int ecspi_read(u32 *d) {
	int maxloop=ECSPI_MAXLOOP;

	DPRINTF("ecspi register [%08x][%08x]\n", ecspi_CONREG, ecspi_STATREG);

	while(maxloop--) {
		/* loop though until data is in the queue */
		if((__REG(ecspi_STATREG)&ECSPI_STATREG_RR)) {
			*d=__REG(ecspi_RXDATA);
			return 1; /* success */
		}
	}
	printf("cspi read failed\n");
	return 0; /* failure */
}
#endif
