/*
 * fsl_arc_udc.c
 *
 * Copyright (C) 2010 Amazon Technologies, Inc.  All rights reserved. 
 *
 * References:
 * DasUBoot/drivers/usb/gadget/omap1510_udc.c, for design and implementation
 * ideas.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.	 See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the
 * Free Software Foundation, Inc.,
 * 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.
 *
 */

#include <common.h>
#include <config.h>
#include <usbdevice.h>
#include <usb_defs.h>
#if defined(CONFIG_PMIC)
#include <pmic.h>
#endif

#include "fsl_arc_udc.h"
#include "ep0.h"

#define ERR(fmt, args...)\
	serial_printf("ERROR : [%s] %s:%d: "fmt,\
				__FILE__,__FUNCTION__,__LINE__, ##args)

//#define __DEBUG_UDC__ 1
#ifdef __DEBUG_UDC__
static char print_debug = 0;
#define DBG(fmt,args...)		\
    if (print_debug)	\
	serial_printf("[%s] %s:%d: "fmt,	\
		      __FILE__,__FUNCTION__,__LINE__, ##args)

//#define __VERBOSE_UDC__ 1
#ifdef __VERBOSE_UDC__
#define VDBG DBG
#else
#define VDBG(fmt,args...)
#endif

#else
#define DBG(fmt,args...)
#define VDBG(fmt,args...)
#endif

#define USB_RESET_TIMEOUT_VAL 200000


static struct usb_device_instance *udc_device;
static struct urb *ep0_urb = NULL;

/* ep0 transfer state */
#define EP0_WAIT_FOR_SETUP          0
#define EP0_DATA_STATE_XMIT         1
#define EP0_DATA_STATE_RECV         2
#define EP0_WAIT_FOR_OUT_STATUS     3

static int ep0_state = EP0_WAIT_FOR_SETUP;

/* structures sent to controller must be properly aligned */
static struct ep_queue_head ep_qh[USB_MAX_PIPES] __attribute__ ((__aligned__(QH_ALIGNMENT)));


#define MAX_DTDS 4
#define MAX_DTD_BYTES 16384

static struct ep_td_struct ep0_td __attribute__ ((__aligned__(DTD_ALIGNMENT)));
static struct ep_td_struct epIN_tds[MAX_DTDS] __attribute__ ((__aligned__(DTD_ALIGNMENT))); 
static struct ep_td_struct epOUT_tds[MAX_DTDS] __attribute__ ((__aligned__(DTD_ALIGNMENT))); 

/* structures to keep track of completed DTDs */
static struct ep_td_struct *ep_list_head[USB_MAX_PIPES];

static struct usb_endpoint_instance *in_epi = NULL;
static struct usb_endpoint_instance *out_epi = NULL;

#if defined(CONFIG_PMIC)
static pmic_charge_type ichrg_value = pmic_charge_none;
#endif
static unsigned int green_led = 0;
static int autochg_en = 1;

#define GREEN_LED_THRESHOLD	4150
#define YELLOW_LED_THRESHOLD	4100

static void fsl_arc_reset_queues(void);

/******************************************************************************
			       Global Linkage
 *****************************************************************************/

#ifdef __DEBUG_UDC__
void fsl_arc_dump_regs(void)
{
    printf("USBID = 0x%x\n", USBID);
    printf("SBUSCFG = 0x%x\n", SBUSCFG);
    printf("DCCPARAMS = 0x%x\n", OTG_DCCPARAMS);
    printf("USBCMD = 0x%x\n", OTG_USBCMD);
    printf("USBSTATUS = 0x%x\n", OTG_USBSTATUS);
    printf("USBINTR = 0x%x\n", OTG_USBINTR);
    printf("DEVICEADDR = 0x%x\n", OTG_DEVICEADDR);
    printf("EPLISTADDR = 0x%x\n", OTG_EPLISTADDR);
    printf("PORTSC1 = 0x%x\n", OTG_PORTSC1);
    printf("USBMODE = 0x%x\n", OTG_USBMODE);
    printf("OTG_EPSETUPST = 0x%x\n", OTG_EPSETUPST);
    printf("OTG_EPPRIME = 0x%x\n", OTG_EPPRIME);
    printf("OTG_EPSTAT = 0x%x\n", OTG_EPSTAT);
    printf("OTG_EPCOMPLETE = 0x%x\n", OTG_EPCOMPLETE);
    printf("EPCTL0 = 0x%x\n", OTG_EPCTL(0));
    printf("EPCTL1 = 0x%x\n", OTG_EPCTL(1));
    printf("OTG_USBCTRL = 0x%x\n", OTG_USBCTRL);
    printf("OTG_USB_MIRROR = 0x%x\n", OTG_USB_MIRROR);
    printf("UTMI_PHY_CTL0 = 0x%x\n", OTG_UTMI_PHY_CTRL0);
    printf("UTMI_PHY_CTL1 = 0x%x\n", OTG_UTMI_PHY_CTRL1);
}

void fsl_arc_dump_qh(void)
{
    int i;
    struct ep_queue_head *p_QH;
    struct ep_td_struct *p_td;
    
    for (i = 0; i < 2; i++) {
	printf("EP%d rx:\n", i);
	p_QH = &ep_qh[(2 * i) + USB_RECV];	
	printf("  mxpkt=%x\n", p_QH->max_pkt_length);
	printf("  curr_dtd=0x%x\n", p_QH->curr_dtd_ptr);
	printf("  next_dtd=0x%x\n", p_QH->next_dtd_ptr);
	printf("  size=0x%x\n", p_QH->size_ioc_int_sts);
	printf("  bufptr0=0x%x\n", p_QH->buff_ptr0);
	printf("  bufptr1=0x%x\n", p_QH->buff_ptr1);
	printf("  bufptr2=0x%x\n", p_QH->buff_ptr2);
	printf("  bufptr3=0x%x\n", p_QH->buff_ptr3);
	printf("  bufptr4=0x%x\n", p_QH->buff_ptr4);
	
	p_td = (struct ep_td_struct *) (p_QH->next_dtd_ptr & EP_QUEUE_HEAD_NEXT_POINTER_MASK);
	if (p_QH->next_dtd_ptr & EP_QUEUE_HEAD_NEXT_TERMINATE)
	    p_td = NULL;

	while (p_td) {
	    printf("  queued TD 0x%x\n", (unsigned int) p_td);
	    printf("    next_dtd=0x%x\n", p_td->next_td_ptr);
	    printf("    size=0x%x\n", p_td->size_ioc_sts);
	    printf("    bufptr0=0x%x\n", p_td->buff_ptr0);
	    printf("    bufptr1=0x%x\n", p_td->buff_ptr1);
	    printf("    bufptr2=0x%x\n", p_td->buff_ptr2);
	    printf("    bufptr3=0x%x\n", p_td->buff_ptr3);
	    printf("    bufptr4=0x%x\n", p_td->buff_ptr4);

	    if (p_td->next_td_ptr & DTD_NEXT_TERMINATE)
		break;

	    p_td = (struct ep_td_struct *) (p_td->next_td_ptr & DTD_ADDR_MASK);	    
	}	    
	
	p_td = (struct ep_td_struct *) (p_QH->curr_dtd_ptr & EP_QUEUE_HEAD_NEXT_POINTER_MASK);
	if (p_QH->curr_dtd_ptr &EP_QUEUE_HEAD_NEXT_TERMINATE)
	    p_td = NULL;

	while (p_td) {
	    printf("  completed TD 0x%x\n", (unsigned int) p_td);
	    printf("    next_dtd=0x%x\n", p_td->next_td_ptr);
	    printf("    size=0x%x\n", p_td->size_ioc_sts);
	    printf("    bufptr0=0x%x\n", p_td->buff_ptr0);
	    printf("    bufptr1=0x%x\n", p_td->buff_ptr1);
	    printf("    bufptr2=0x%x\n", p_td->buff_ptr2);
	    printf("    bufptr3=0x%x\n", p_td->buff_ptr3);
	    printf("    bufptr4=0x%x\n", p_td->buff_ptr4);

	    if (p_td->next_td_ptr & DTD_NEXT_TERMINATE)
		break;
	    
	    p_td = (struct ep_td_struct *) (p_td->next_td_ptr & DTD_ADDR_MASK);
	}

	printf("EP%d tx:\n", i);
	p_QH = &ep_qh[(2 * i) + USB_SEND];	
	printf("  mxpkt=%x\n", p_QH->max_pkt_length);
	printf("  curr_dtd=0x%x\n", p_QH->curr_dtd_ptr);
	printf("  next_dtd=0x%x\n", p_QH->next_dtd_ptr);
	printf("  size=0x%x\n", p_QH->size_ioc_int_sts);
	printf("  bufptr0=0x%x\n", p_QH->buff_ptr0);
	printf("  bufptr1=0x%x\n", p_QH->buff_ptr1);
	printf("  bufptr2=0x%x\n", p_QH->buff_ptr2);
	printf("  bufptr3=0x%x\n", p_QH->buff_ptr3);
	printf("  bufptr4=0x%x\n", p_QH->buff_ptr4);

	p_td = (struct ep_td_struct *) (p_QH->next_dtd_ptr & EP_QUEUE_HEAD_NEXT_POINTER_MASK);
	if (p_QH->next_dtd_ptr & EP_QUEUE_HEAD_NEXT_TERMINATE)
	    p_td = NULL;

	while (p_td) {
	    printf("  queued TD 0x%x\n", (unsigned int) p_td);
	    printf("    next_dtd=0x%x\n", p_td->next_td_ptr);
	    printf("    size=0x%x\n", p_td->size_ioc_sts);
	    printf("    bufptr0=0x%x\n", p_td->buff_ptr0);
	    printf("    bufptr1=0x%x\n", p_td->buff_ptr1);
	    printf("    bufptr2=0x%x\n", p_td->buff_ptr2);
	    printf("    bufptr3=0x%x\n", p_td->buff_ptr3);
	    printf("    bufptr4=0x%x\n", p_td->buff_ptr4);

	    if (p_td->next_td_ptr & DTD_NEXT_TERMINATE)
		break;

	    p_td = (struct ep_td_struct *) (p_td->next_td_ptr & DTD_ADDR_MASK);	    
	}	    
	
	p_td = (struct ep_td_struct *) (p_QH->curr_dtd_ptr & EP_QUEUE_HEAD_NEXT_POINTER_MASK);
	if (p_QH->curr_dtd_ptr &EP_QUEUE_HEAD_NEXT_TERMINATE)
	    p_td = NULL;

	while (p_td) {
	    printf("  completed TD 0x%x\n", (unsigned int) p_td);
	    printf("    next_dtd=0x%x\n", p_td->next_td_ptr);
	    printf("    size=0x%x\n", p_td->size_ioc_sts);
	    printf("    bufptr0=0x%x\n", p_td->buff_ptr0);
	    printf("    bufptr1=0x%x\n", p_td->buff_ptr1);
	    printf("    bufptr2=0x%x\n", p_td->buff_ptr2);
	    printf("    bufptr3=0x%x\n", p_td->buff_ptr3);
	    printf("    bufptr4=0x%x\n", p_td->buff_ptr4);

	    if (p_td->next_td_ptr & DTD_NEXT_TERMINATE)
		break;
	    
	    p_td = (struct ep_td_struct *) (p_td->next_td_ptr & DTD_ADDR_MASK);
	}

	printf("\n");
    }
}
#endif

void fsl_arc_dr_ep_setup(unsigned char ep_num, unsigned char dir, unsigned char ep_type)
{
    unsigned int tmp_epctrl = 0;

    tmp_epctrl = OTG_EPCTL(ep_num);

    if (dir) {
	if (ep_num)
	    tmp_epctrl |= EPCTRL_TX_DATA_TOGGLE_RST;
	tmp_epctrl |= EPCTRL_TX_ENABLE;
	tmp_epctrl |= ((unsigned int)(ep_type)
		       << EPCTRL_TX_EP_TYPE_SHIFT);
    } else {
	if (ep_num)
	    tmp_epctrl |= EPCTRL_RX_DATA_TOGGLE_RST;
	tmp_epctrl |= EPCTRL_RX_ENABLE;
	tmp_epctrl |= ((unsigned int)(ep_type)
		       << EPCTRL_RX_EP_TYPE_SHIFT);
    }

    OTG_EPCTL(ep_num) = tmp_epctrl;

}

/*------------------------------------------------------------------
* struct_ep_qh_setup(): set the Endpoint Capabilites field of QH
 * @zlt: Zero Length Termination Select (1: disable; 0: enable)
 * @mult: Mult field
 ------------------------------------------------------------------*/
static void fsl_arc_struct_ep_qh_setup(unsigned char ep_num,
		unsigned char dir, unsigned char ep_type,
		unsigned int max_pkt_len,
		unsigned int zlt, unsigned char mult)
{
    struct ep_queue_head *p_QH = &ep_qh[(2 * ep_num) + dir];
    unsigned int tmp = 0;

    /* set the Endpoint Capabilites in QH */
    switch (ep_type) {
      case USB_ENDPOINT_XFER_CONTROL:
	/* Interrupt On Setup (IOS). for control ep  */
	tmp = (max_pkt_len << EP_QUEUE_HEAD_MAX_PKT_LEN_POS)
	    | EP_QUEUE_HEAD_IOS;
	break;
      case USB_ENDPOINT_XFER_ISOC:
	tmp = (max_pkt_len << EP_QUEUE_HEAD_MAX_PKT_LEN_POS)
	    | (mult << EP_QUEUE_HEAD_MULT_POS);
	break;
      case USB_ENDPOINT_XFER_BULK:
      case USB_ENDPOINT_XFER_INT:
	tmp = max_pkt_len << EP_QUEUE_HEAD_MAX_PKT_LEN_POS;
	break;
      default:
	ERR("error ep type is %d\n", ep_type);
	return;
    }
    if (zlt)
	tmp |= EP_QUEUE_HEAD_ZLT_SEL;
    p_QH->max_pkt_length = tmp;
    p_QH->next_dtd_ptr = EP_QUEUE_HEAD_NEXT_TERMINATE;
    p_QH->size_ioc_int_sts = 0;

    return;
}

/* Setup qh structure and ep register for ep0. */
static void fsl_arc_ep0_setup(void)
{
    /* the intialization of an ep includes: fields in QH, Regs,
     * fsl_ep struct */

    DBG("ep0setup begin\n");

    fsl_arc_struct_ep_qh_setup(0, USB_RECV, USB_ENDPOINT_XFER_CONTROL,
			       USB_MAX_CTRL_PAYLOAD, 0, 0);
    fsl_arc_struct_ep_qh_setup(0, USB_SEND, USB_ENDPOINT_XFER_CONTROL,
			       USB_MAX_CTRL_PAYLOAD, 0, 0);
    fsl_arc_dr_ep_setup(0, USB_RECV, USB_ENDPOINT_XFER_CONTROL);
    fsl_arc_dr_ep_setup(0, USB_SEND, USB_ENDPOINT_XFER_CONTROL);

    ep0_state = EP0_WAIT_FOR_SETUP;

    DBG("ep0setup complete\n");
}

static int fsl_arc_reset_controller(void) 
{
    unsigned int tmp;
    unsigned int tmout = 0;

    // reset the controller
    tmp = OTG_USBCMD;
    tmp &= ~USB_CMD_RUN_STOP;
    OTG_USBCMD = tmp;

    tmp = OTG_USBCMD;
    tmp |= USB_CMD_CTRL_RESET;
    OTG_USBCMD = tmp;

    DBG("Waiting for USB controller reset...");
    while (OTG_USBCMD & USB_CMD_CTRL_RESET) {
	tmout++;
	if (tmout > USB_RESET_TIMEOUT_VAL) {
	    ERR("\nError: USB reset timed out!\n");
	    return -1;
	}	   
    }
    DBG("done. (%d)\n", tmout);

    return 0;
}

int fsl_arc_init(void)
{
    unsigned int tmp;

    DBG("%s: begin\n", __FUNCTION__);

    fsl_arc_reset_controller();

#if defined(CONFIG_MX51) || defined(CONFIG_MX50)
    /* not using OC */
    OTG_UTMI_PHY_CTRL0 |= USB_UTMI_PHYCTRL_OC_DIS;

    OTG_USBCTRL &= ~UCTRL_OPM;	/* OTG Power Mask */
    OTG_USBCTRL &= ~UCTRL_OWIE;	/* OTG Wakeup Intr Disable */

    /* set UTMI xcvr */
    tmp = OTG_PORTSC1 & ~(PORTSCX_PHY_TYPE_SEL | PORTSCX_PORT_WIDTH);
    tmp |= (PORTSCX_PTW_16BIT | PORTSCX_PTS_UTMI);
    OTG_PORTSC1 = tmp;

    /* Set the PHY clock to 19.2MHz */
    OTG_UTMI_PHY_CTRL1 &= ~USB_UTMI_PHYCTRL2_PLLDIV_MASK;
    OTG_UTMI_PHY_CTRL1 |= 0x01;

    /* Enable UTMI interface in PHY control Reg */
    OTG_UTMI_PHY_CTRL0 &= ~USB_UTMI_PHYCTRL_UTMI_ENABLE;
    OTG_UTMI_PHY_CTRL0 |= USB_UTMI_PHYCTRL_UTMI_ENABLE;

    /* reset again */
    fsl_arc_reset_controller();
#endif

    // put the controller into device mode
    OTG_USBMODE = USB_MODE_CTRL_MODE_DEVICE | USB_MODE_SETUP_LOCK_OFF;

    VDBG("clr setup\n");

    // clear setup status
    OTG_USBSTATUS = 0;

    DBG("write queue head (0x%x)\n", (unsigned int) &ep_qh);

    // init the local completed dtd queue
    memset(ep_list_head, 0, sizeof(ep_list_head));

    // write EP queue head
    tmp = (unsigned int) &ep_qh;
    tmp &= USB_EP_LIST_ADDRESS_MASK;
    OTG_EPLISTADDR = tmp;

    DBG("config phy\n");
    tmp = OTG_PORTSC1;
    tmp &= ~(PORTSCX_PHY_TYPE_SEL | PORTSCX_PORT_WIDTH);
    tmp |= (PORTSCX_PTW_16BIT | PORTSCX_PTS_UTMI);
    OTG_PORTSC1 = tmp;

    return 0;
}

/* udc_init
 *
 * Do initial bus gluing
 */
int udc_init (void)
{
    int ret;

    ret = fsl_arc_init();
    if (ret)
	return ret;

    DBG("setup ep0\n");

    fsl_arc_ep0_setup();

    return 0;
}

/*-------------------------------------------------------------------------*/
static int fsl_arc_queue_td(unsigned int ep_num, unsigned int ep_dir, struct ep_td_struct *ep_td)
{
    int i = (ep_num * 2) + ep_dir;
    unsigned int tmp, bitmask;
    struct ep_queue_head *dQH = &ep_qh[i];

    DBG("%s: begin\n", __FUNCTION__);

    bitmask = ep_dir
	? (1 << (ep_num + 16))
	: (1 << (ep_num));

#if 0 // BEN TODO
    /* check if the pipe is empty */
    if (!(list_empty(&ep->queue))) {
	unsigned int tmp_stat;
	/* Add td to the end */
	struct fsl_req *lastreq;
	lastreq = list_entry(ep->queue.prev, struct fsl_req, queue);
	lastreq->tail->next_td_ptr =
	    cpu_to_hc32(req->head->td_dma & DTD_ADDR_MASK);
	/* Read prime bit, if 1 goto done */
	if (fsl_readl(&dr_regs->endpointprime) & bitmask)
	    goto out;

	do {
	    /* Set ATDTW bit in USBCMD */
	    temp = fsl_readl(&dr_regs->usbcmd);
	    fsl_writel(temp | USB_CMD_ATDTW, &dr_regs->usbcmd);

	    /* Read correct status bit */
	    tmp_stat = fsl_readl(&dr_regs->endptstatus) & bitmask;

	} while (!(fsl_readl(&dr_regs->usbcmd) & USB_CMD_ATDTW));

	/* Write ATDTW bit to 0 */
	temp = fsl_readl(&dr_regs->usbcmd);
	fsl_writel(temp & ~USB_CMD_ATDTW, &dr_regs->usbcmd);

	if (tmp_stat)
	    goto out;
    }
#endif
    DBG("writing td: 0x%x\n", (unsigned int) ep_td);

    /* Write dQH next pointer and terminate bit to 0 */
    tmp = ((unsigned int) ep_td) & EP_QUEUE_HEAD_NEXT_POINTER_MASK;
    dQH->next_dtd_ptr = tmp;

    /* Clear active and halt bit */
    dQH->size_ioc_int_sts &= ~(EP_QUEUE_HEAD_STATUS_ACTIVE
			       | EP_QUEUE_HEAD_STATUS_HALT);
	

    DBG("prime bitmask: 0x%x\n", bitmask);
    /* Prime endpoint by writing 1 to ENDPTPRIME */
    OTG_EPPRIME = bitmask;

    DBG("%s: complete\n", __FUNCTION__);
    
    return 0;
}

static void fsl_arc_build_dtd(struct ep_td_struct *dtd, unsigned char *sndbuf, unsigned int length, int is_last)
{
    unsigned int tmp;
    
    DBG("%s: begin\n", __FUNCTION__);

    // clear out DTD struct
    memset(dtd, 0, sizeof(struct ep_td_struct));

    DBG("  buf=0x%x len=%d\n", (unsigned int) sndbuf, length);

    // set the buffer pointers
    dtd->buff_ptr0 = (unsigned int) sndbuf;
    dtd->buff_ptr1 = (unsigned int) sndbuf + 0x1000;
    dtd->buff_ptr2 = (unsigned int) sndbuf + 0x2000;
    dtd->buff_ptr3 = (unsigned int) sndbuf + 0x3000;
    dtd->buff_ptr4 = (unsigned int) sndbuf + 0x4000;

    // set length
    tmp = (length << DTD_LENGTH_BIT_POS) | DTD_STATUS_ACTIVE;

    if (is_last)
	tmp |= DTD_IOC;

    dtd->size_ioc_sts = tmp;
    dtd->next_td_ptr = DTD_NEXT_TERMINATE;

    // BEN DEBUG
    VDBG("  TD 0x%x\n", (unsigned int) dtd);
    VDBG("    next_dtd=0x%x\n", dtd->next_td_ptr);
    VDBG("    size=0x%x\n", dtd->size_ioc_sts);
    VDBG("    bufptr0=0x%x\n", dtd->buff_ptr0);
    VDBG("    bufptr1=0x%x\n", dtd->buff_ptr1);
    VDBG("    bufptr2=0x%x\n", dtd->buff_ptr2);
    VDBG("    bufptr3=0x%x\n", dtd->buff_ptr3);
    VDBG("    bufptr4=0x%x\n", dtd->buff_ptr4);

    VDBG("%s: complete\n", __FUNCTION__);
}

/* Stall endpoint */
static void fsl_arc_stall_ep (unsigned int ep_addr)
{
    int ep_num = ep_addr & USB_ENDPOINT_NUMBER_MASK;

    DBG ("stall ep%d\n", ep_num);

    if (ep_num == 0)
    {
	/* stall ctrl ep */
	/* must set tx and rx to stall at the same time */
	OTG_EPCTL(0) |= EPCTRL_TX_EP_STALL | EPCTRL_RX_EP_STALL;

	ep0_state = EP0_WAIT_FOR_SETUP;
    }
}

static void fsl_arc_ep0_prime_status(int direction)
{
    fsl_arc_build_dtd(&ep0_td, NULL, 0, 1);
    fsl_arc_queue_td(0, direction, &ep0_td);	
}

/* Process a DTD completion interrupt */
static int fsl_arc_dtd_complete_irq(void)
{
    u32 bit_pos;
    int i, ep_num, direction, bit_mask;
    struct ep_queue_head *p_QH;
    struct ep_td_struct *p_td;

    /* Clear the bits in the register */
    bit_pos = OTG_EPCOMPLETE;
    if (!bit_pos) {
	ERR("no eps complete!\n");
	return 0;
    }

    OTG_EPCOMPLETE = bit_pos;

    for (i = 0; i < USB_MAX_PIPES; i++) {
	ep_num = i >> 1;
	direction = i % 2;

	bit_mask = 1 << (ep_num + 16 * direction);

	if (!(bit_pos & bit_mask))
	    continue;

	DBG("%s: ep=%d dir=%d\n", __FUNCTION__, ep_num, direction);

	if (ep_num == 0) {
	    p_QH = &ep_qh[i];
	    p_td = (struct ep_td_struct *) (p_QH->curr_dtd_ptr & EP_QUEUE_HEAD_NEXT_POINTER_MASK);
	    if (p_QH->curr_dtd_ptr & EP_QUEUE_HEAD_NEXT_TERMINATE)
		p_td = NULL;
	} else {
	    p_td = ep_list_head[i];
	}

	while (p_td) {
	    if (p_td->size_ioc_sts & DTD_ERROR_MASK) {

		ERR("send error: 0x%x!\n", p_td->size_ioc_sts & DTD_ERROR_MASK);
		
		return -1;
	    }
	    else if (p_td->size_ioc_sts & DTD_STATUS_ACTIVE) {
		DBG("%s: request incomplete!\n", __FUNCTION__);
		return 0;
	    }
	    else if (direction == USB_SEND && (p_td->size_ioc_sts & DTD_PACKET_SIZE) >> DTD_LENGTH_BIT_POS) {
		ERR("tx dtd remaining length non-zero! (0x%x)\n", (p_td->size_ioc_sts & DTD_PACKET_SIZE) >> DTD_LENGTH_BIT_POS);
		return -1;
	    }
	    else {
		DBG ("%s: Action on pipe %d successful\n", __FUNCTION__, i);

		if (ep_num == 0) {
		    switch (ep0_state) {
		      case EP0_DATA_STATE_RECV:

			// send ACK packet
			DBG("  sending ACK\n");
			fsl_arc_ep0_prime_status(USB_SEND);
			ep0_state = EP0_WAIT_FOR_SETUP;
			break;

		      case EP0_DATA_STATE_XMIT:

			// wait for ACK from host
			DBG("  wait for ACK\n");
			fsl_arc_ep0_prime_status(USB_RECV);
			ep0_state = EP0_WAIT_FOR_OUT_STATUS;
			break;
		    
		      case EP0_WAIT_FOR_OUT_STATUS:
		      default:

			if (udc_device->address && !OTG_DEVICEADDR) {
			    /* Move to the addressed state */
			    OTG_DEVICEADDR = (udc_device->address << USB_DEVICE_ADDRESS_BIT_POS);
			    /* tell usb driver about address state */
			    usbd_device_event_irq (udc_device, DEVICE_ADDRESS_ASSIGNED, 0);
			}

			DBG("  done.\n");
			ep0_state = EP0_WAIT_FOR_SETUP;
		    }
		} else if (direction == USB_SEND) {		 
   
		    in_epi->last = in_epi->tx_urb->actual_length;
		    usbd_tx_complete(in_epi);
		    DBG("  tx done\n");
		    break;

		} else if (direction == USB_RECV) {
		    int bytes_rcvd;
		    struct ep_td_struct *next_td;

		    // take current TD off the head of the queue
		    next_td = (struct ep_td_struct *) (p_td->next_td_ptr & DTD_ADDR_MASK);
		    ep_list_head[i] = next_td;

		    /* bytes received is the difference between the max packet size and 
		       the number of bytes currently remaining in the td */
		    // BEN TODO
		    bytes_rcvd = MAX_DTD_BYTES - 
			((p_td->size_ioc_sts & DTD_PACKET_SIZE) >> DTD_LENGTH_BIT_POS);
		    usbd_rcv_complete(out_epi, bytes_rcvd, 0);

		    DBG("  rcvd %d bytes\n", bytes_rcvd);

		    p_td->next_td_ptr = DTD_NEXT_TERMINATE;
		    p_td = next_td;
		    continue;
		}
	    }

	    if (p_td->next_td_ptr & DTD_NEXT_TERMINATE)
		break;

	    p_td = (struct ep_td_struct *) (p_td->next_td_ptr & EP_QUEUE_HEAD_NEXT_POINTER_MASK);
	    DBG("%s: next td=%x\n",  __FUNCTION__, (unsigned int) p_td);
	}
    }

    return 0;
}

static void fsl_arc_setup_received_irq(void)
{
    struct ep_queue_head *qh;
    unsigned char *datap = (unsigned char *) &ep0_urb->device_request;
    int ret;
    unsigned int tmp;

    qh = &ep_qh[0];

    DBG("reading setup packet..");

    /* Read out SETUP packet safely */
    /* Clear bit in ENDPTSETUPSTAT */
    tmp = OTG_EPSETUPST;
    OTG_EPSETUPST = tmp;

    /* while a hazard exists when setup package arrives */
    do {
	/* Set Setup Tripwire */
	OTG_USBCMD |= USB_CMD_SUTW;

	/* Copy the setup packet to local buffer */
	memcpy(datap, (u8 *) qh->setup_buffer, 8);

    } while (!(OTG_USBCMD & USB_CMD_SUTW));

    /* Clear Setup Tripwire */
    OTG_USBCMD &= ~USB_CMD_SUTW;

    DBG("done\n");

    /* Try to process setup packet */
    if (ep0_recv_setup (ep0_urb)) {
	/* Not a setup packet, stall next EP0 transaction */
	fsl_arc_stall_ep (0);
	printf("%s: unhandled setup packet, stalling\n", __FUNCTION__);
	return;
    }

    /* Check direction */
    if ((ep0_urb->device_request.bmRequestType & USB_REQ_DIRECTION_MASK)
	== USB_REQ_HOST2DEVICE) {
	DBG ("control write on EP0\n");

	switch (ep0_urb->device_request.bRequest) {
	  case USB_REQ_SET_ADDRESS:

	    /* Send ACK to host */
	    fsl_arc_ep0_prime_status(USB_SEND);
	    ep0_state = EP0_WAIT_FOR_SETUP;

	    DBG("addr=%d\n", udc_device->address);

	    return;

	  case USB_REQ_SET_CONFIGURATION:

	    if (ep0_urb->device_request.wValue) {
		/* tell usb driver a connection has been established */
		usbd_device_event_irq (udc_device, DEVICE_CONFIGURED, 0);

		/* start charging at 500 mA if high speed */
		if (udc_device->speed == USB_SPEED_HIGH) {

#if defined(CONFIG_PMIC)
		    if (!ichrg_value) {
			ret = pmic_start_charging(pmic_charge_usb_high, autochg_en);
		    } else {
			ret = pmic_set_chg_current(pmic_charge_usb_high);
		    }
		    if (ret) {
			ichrg_value = pmic_charge_usb_high;
			printf("USB configured.\n");
		    } else {
			ichrg_value = 0;
		    }
#endif		    
		}

	    } else {
	
		/* flush all non-control endpoints */
		OTG_EPFLUSH = 0xfffefffe;

		/* reset ep structures */
		fsl_arc_reset_queues();

		/* tell usb driver to reinit eps */
		usbd_device_event_irq (udc_device, DEVICE_DE_CONFIGURED, 0);

#if defined(CONFIG_PMIC)
		/* charge at the low rate if deconfigured */
		if (ichrg_value >= pmic_charge_usb_high) {
		    ret = pmic_set_chg_current(pmic_charge_host);
		    ichrg_value = (ret) ? pmic_charge_host : 0;
		}
#endif
		printf("USB deconfigured.\n");
	    }

	    /* Send ACK to host */
	    fsl_arc_ep0_prime_status(USB_SEND);
	    ep0_state = EP0_WAIT_FOR_SETUP;

	    return;
	}

	if (le16_to_cpu (ep0_urb->device_request.wLength)) {
	    /* We don't support control write data stages.
	     * The only standard control write request with a data
	     * stage is SET_DESCRIPTOR, and ep0_recv_setup doesn't
	     * support that so we just stall those requests.  A
	     * function driver might support a non-standard
	     * write request with a data stage, but it isn't
	     * obvious what we would do with the data if we read it
	     * so we'll just stall it.  It seems like the API isn't
	     * quite right here.
	     */
#if 0
	    /* Here is what we would do if we did support control
	     * write data stages.
	     */
	    ep0_urb->actual_length = 0;
	    outw (0, UDC_EP_NUM);
	    /* enable the EP0 rx FIFO */
	    outw (UDC_Set_FIFO_En, UDC_CTRL);
#else
	    /* Stall this request */
	    ERR ("Stalling unsupported EP0 control write data "
		    "stage.\n");
	    fsl_arc_stall_ep (0);
#endif
	} else {
	    /* Send ACK to host */
	    fsl_arc_ep0_prime_status(USB_SEND);
	    ep0_state = EP0_WAIT_FOR_SETUP;
	}
    } else {

	DBG ("control read on EP0");
	/* The ep0_recv_setup function has already placed our response
	 * packet data in ep0_urb->buffer and the packet length in
	 * ep0_urb->actual_length.
	 */
		
#if 0 // BEN DEBUG
	{
	    int i;
	    DBG("snd pkt (%d)", ep0_urb->actual_length);
	    for (i = 0; i < ep0_urb->actual_length; i++) {
		if (!(i % 16))
		    DBG("\n0x%02x: ", i);
		DBG("%02x ", ep0_urb->buffer[i]);
	    }
	    DBG("\n");
	}
#endif
	fsl_arc_build_dtd(&ep0_td, ep0_urb->buffer, ep0_urb->actual_length, 1);
	fsl_arc_queue_td(0, USB_SEND, &ep0_td);

	ep0_state = EP0_DATA_STATE_XMIT;
    }

    DBG ("<- Leaving device setup\n");    
}

static void fsl_arc_reset_queues(void) 
{
    int i;
    struct ep_queue_head *p_QH;

    // reset non-setup queues
    for (i = 2; i < USB_MAX_PIPES; i++) {
	p_QH = &ep_qh[i];

	p_QH->max_pkt_length = 0;
	p_QH->size_ioc_int_sts = 0;
	p_QH->next_dtd_ptr = EP_QUEUE_HEAD_NEXT_TERMINATE;
    }
}

/* Process reset interrupt */
static void fsl_arc_reset_irq(void)
{
	u32 tmp;
	unsigned long timeout;
	
	/* Clear the device address */
	OTG_DEVICEADDR &= ~USB_DEVICE_ADDRESS_MASK;
	udc_device->address = 0;

	/* reset speed val */
	udc_device->speed = USB_SPEED_UNKNOWN;

	/* Clear all the setup token semaphores */
	tmp = OTG_EPSETUPST;
	OTG_EPSETUPST = tmp;

	/* Clear all the endpoint complete status bits */
	tmp = OTG_EPCOMPLETE;
	OTG_EPCOMPLETE = tmp;

	timeout = 0;
	while (OTG_EPPRIME) {
	    /* Wait until all endptprime bits cleared */
	    if (timeout++ > 20000) {
		ERR("EP reset timeout!\n");
		break;
	    }
	}

	/* Write 1s to the flush register */
	OTG_EPFLUSH = 0xffffffff;

	if (OTG_PORTSC1 & PORTSCX_PORT_RESET) {
	    DBG ("Bus reset PORTSC=0x%x\n", OTG_PORTSC1);
		/* Bus is reseting */
		/* Reset all the queues, include XD, dTD, EP queue
		 * head and TR Queue */
		fsl_arc_reset_queues();
	} else {
	    DBG ("Controller reset\n");
#ifndef __DEBUG_UDC__  // debug printfs take too long for reset
		/* initialize usb hw reg except for regs for EP, not
		 * touch usbintr reg */
		fsl_arc_init();

		/* Reset all internal used Queues */
		fsl_arc_reset_queues();

		fsl_arc_ep0_setup();

		/* Enable DR IRQ reg, Set Run bit, change udc state */
		udc_enable(udc_device);
#endif
	}

	// set correct state
	usbd_device_event_irq (udc_device, DEVICE_RESET, 0);
}

static void fsl_arc_check_voltage(void) {
#if defined(CONFIG_PMIC)
    unsigned short voltage;

    int ret = pmic_adc_read_voltage(&voltage);
    if (ret) {
	printf("Battery voltage: %d mV\n", voltage);

	if (voltage < CONFIG_BOOT_AUTOCHG_VOLTAGE) {
	    autochg_en = 0;
	} else {
	    if (autochg_en == 0) {
		/* unset auto-voltage after voltage 
		   is above lobat */
		pmic_set_autochg(0);
	    }
	
	    autochg_en = 1;
	}
    } else {
	autochg_en = 1;
	printf("Battery voltage read fail!\n");
    }

    /* see if we're charging, enable green led if we're "done" */
    if (pmic_charging()) {
	if (!green_led && voltage > GREEN_LED_THRESHOLD) {
	    pmic_enable_green_led(1);
	    green_led = 1;
	    DBG("LED green!\n");

	} else if (green_led && voltage < YELLOW_LED_THRESHOLD) {
	    pmic_enable_green_led(0);
	    green_led = 0;
	    DBG("LED yellow!\n");

	    pmic_charge_restart();
	}
    } else if (green_led) {
	pmic_enable_green_led(0);
	green_led = 0;
    }
#endif		    
}

static void fsl_arc_check_charger(void) {
#if defined(CONFIG_PMIC)
    int ret;
    unsigned int tmp;

    /* check to see if a charger has been plugged/unplugged */
    tmp = OTG_PORTSC1;

    /* no cable connected */
    if (!(tmp & PORTSCX_CURRENT_CONNECT_STATUS)) {
	if (ichrg_value) {
	    /* charger has been disconnected */
	    pmic_set_chg_current(0);
	    ichrg_value = 0;

	    printf("Charger disconnect\n");
	} else {
	    return;
	}

    } else {
    
	if ((tmp & PORTSCX_LINE_STATUS_BITS) == PORTSCX_LINE_STATUS_UNDEF &&
	    (ichrg_value < pmic_charge_wall)) {

	    printf("Connected to charger!\n");
	    
	    if (ichrg_value == pmic_charge_none) {
		ret = pmic_start_charging(pmic_charge_wall, autochg_en);
	    } else {
		ret = pmic_set_chg_current(pmic_charge_wall);
	    }
	    ichrg_value = (ret) ? pmic_charge_wall : pmic_charge_none;

	} 
    }
#endif
}

static unsigned int bat_check_timeout = 0;

/* udc_irq
 *
 * Poll for whatever events may have occured
 */
void udc_irq (void)
{
    unsigned int irq_src;

    bat_check_timeout++;

    fsl_arc_check_charger();
    
    /* check every 20s or so */
    if (bat_check_timeout == 10000000) 
    {
	fsl_arc_check_voltage();
	bat_check_timeout = 0;
    }

    irq_src = OTG_USBSTATUS;
    if (!(irq_src & OTG_USBINTR))
	return;

    DBG ("IRQ: 0x%x\n", irq_src);

    /* clear status bits */
    OTG_USBSTATUS = irq_src;

    if (irq_src & USB_STS_RESET) {
	DBG ("  Packet reset\n");
	fsl_arc_reset_irq();

	// ignore any other interrupts
	return;
    }

    if (irq_src & USB_STS_SUSPEND) {

	DBG ("  Packet suspend\n");
	usbd_device_event_irq (udc_device, DEVICE_BUS_INACTIVE, 0);
    }

    /* USB Interrupt */
    if (irq_src & USB_STS_INT) {
	DBG ("  Packet int (setup=0x%x) (epcmpl=0x%x)\n", OTG_EPSETUPST, OTG_EPCOMPLETE);

	// BEN - handle complete events first
	if (OTG_EPCOMPLETE) {
	    DBG("  TD completed\n");
	    fsl_arc_dtd_complete_irq();	    
	}

	if (OTG_EPSETUPST & EP_SETUP_STATUS_EP0) {
	    DBG("  EP0 packet rcvd\n");
	    fsl_arc_setup_received_irq();
	} 
    }

    if (irq_src & USB_STS_PORT_CHANGE) {
	DBG ("  Packet port change portsc=0x%x\n", OTG_PORTSC1);

	if (!(OTG_PORTSC1 & (PORTSCX_PORT_RESET | PORTSCX_PORT_SUSPEND))) {
	    unsigned int speed;

	    /* Get the speed */
	    speed = (OTG_PORTSC1 & PORTSCX_PORT_SPEED_MASK);
	    switch (speed) {
	      case PORTSCX_PORT_SPEED_HIGH:
		udc_device->speed = USB_SPEED_HIGH;
		printf("USB speed: HIGH\n");
		break;
	      case PORTSCX_PORT_SPEED_FULL:
		udc_device->speed = USB_SPEED_FULL;
		printf("USB speed: FULL\n");
		break;
	      case PORTSCX_PORT_SPEED_LOW:
		udc_device->speed = USB_SPEED_LOW;
		printf("USB speed: LOW\n");
		break;
	      default:
		udc_device->speed = USB_SPEED_UNKNOWN;
		ERR("IRQ - invalid USB speed\n");
		break;
	    }

	    /* set charger if we're connected */
	    if (udc_device->speed != USB_SPEED_UNKNOWN) {
#if defined(CONFIG_PMIC)
		int ret;

		if (ichrg_value == pmic_charge_none) {
		    ret = pmic_start_charging(pmic_charge_host, autochg_en);
		} else {
		    ret = pmic_set_chg_current(pmic_charge_host);
		}
		ichrg_value = (ret) ? pmic_charge_host : pmic_charge_none;
#endif
		printf("Connected to USB host!\n");
	    }
	}
    }

    if (irq_src & USB_STS_ERR) {
	ERR("Packet data err\n");
    }

    if (irq_src & USB_STS_SYS_ERR) {
	ERR("Packet sys err\n");
    }
}

static int fsl_arc_urb_to_td(struct usb_endpoint_instance *epi, int direction) 
{
    int ep_addr = epi->endpoint_address;
    int ep_num = (ep_addr & USB_ENDPOINT_NUMBER_MASK);
    struct ep_td_struct *tdq, *td, *prev_td = NULL;
    unsigned char *dtd_buf;
    unsigned int dtd_bytes;
    int len;
    int is_last_td;
    int dtd_num = 0;

    DBG("%s: begin\n", __FUNCTION__);

    if (direction == USB_SEND) {
	tdq = epIN_tds;
	len = epi->tx_urb->actual_length;
	dtd_buf = epi->tx_urb->buffer;
    } else {
	tdq = epOUT_tds;
	len = epi->rcv_urb->buffer_length;
	dtd_buf = epi->rcv_urb->buffer;
    }

    do {
	td = &(tdq[dtd_num++]);

	if (ep_list_head[(ep_num * 2) + direction] == NULL)
	    ep_list_head[(ep_num * 2) + direction] = td;

	DBG("  dtd=0x%x len=%d dir=%s\n", 
	       (unsigned int) td, len, (direction == USB_SEND) ? "IN" : "OUT");
	dtd_bytes = MIN(len, MAX_DTD_BYTES);

	DBG("  q %d bytes\n", dtd_bytes);

	/* queue dtd */
	is_last_td = ((len - dtd_bytes) == 0);
	fsl_arc_build_dtd(td, dtd_buf, dtd_bytes, is_last_td);

	dtd_buf += dtd_bytes;
	len -= dtd_bytes;

	if (prev_td) {
	    prev_td->next_td_ptr = (unsigned int) td;
	    DBG("     next=0x%x\n", (unsigned int) td);
	}

	prev_td = td;

    } while (len && dtd_num < MAX_DTDS);

    fsl_arc_queue_td(ep_num, direction, tdq);

    DBG("%s: complete\n", __FUNCTION__);

    return 0;
}

/* udc_endpoint_read
 *
 * Set up a receive on an endpoint
 */
int udc_endpoint_read (struct usb_endpoint_instance *epi) 
{
    return fsl_arc_urb_to_td(epi, USB_RECV);
}

/* udc_endpoint_write
 *
 * Write some data to an endpoint
 */
int udc_endpoint_write (struct usb_endpoint_instance *epi)
{
    return fsl_arc_urb_to_td(epi, USB_SEND);
}


/* udc_setup_ep
 *
 * Associate U-Boot software endpoints to endpoint parameter ram
 * Isochronous endpoints aren't yet supported!
 */
void udc_setup_ep (struct usb_device_instance *device, unsigned int ep,
		   struct usb_endpoint_instance *epi)
{
    int ep_addr = epi->endpoint_address;
    int ep_num = (ep_addr & USB_ENDPOINT_NUMBER_MASK);

    DBG("%s: begin\n", __FUNCTION__);

    if (!epi || (ep >= USB_MAX_PIPES)) {
	ERR("%s: invalid endpoint!\n", __FUNCTION__);
	return;
    }

    if (ep == 0) {
	// BEN TODO - use these values for ep0 urb
    } else if ((ep_addr & USB_ENDPOINT_DIR_MASK) == USB_DIR_IN) {

	DBG("  set up IN ep%d\n", ep_num); 
	in_epi = epi;
	
	/* set up IN ep registers */
	fsl_arc_struct_ep_qh_setup(ep_num, USB_SEND, epi->tx_attributes,
			       epi->tx_packetSize, 1, 0);
	fsl_arc_dr_ep_setup(ep_num, USB_SEND, epi->tx_attributes);

    } else {

	DBG("  set up OUT ep%d\n", ep_num); 
	out_epi = epi;

	/* set up OUT ep structure */	
	fsl_arc_struct_ep_qh_setup(ep_num, USB_RECV, epi->rcv_attributes,
				   epi->rcv_packetSize, 1, 0);
	fsl_arc_dr_ep_setup(ep_num, USB_RECV, epi->rcv_attributes);
    }

    DBG("%s: complete\n", __FUNCTION__);
}

/* udc_connect
 *
 * Move state, switch on the USB
 */
void udc_connect (void)
{
    DBG("%s: begin\n", __FUNCTION__);

    DBG("%s: complete\n", __FUNCTION__);
}

/* udc_disconnect
 *
 * Disconnect is not used but, is included for completeness
 */
void udc_disconnect (void)
{
    DBG("%s: begin\n", __FUNCTION__);

    DBG("%s: complete\n", __FUNCTION__);
}

/* udc_enable
 *
 * Grab an EP0 URB, register interest in a subset of USB events
 */
void udc_enable (struct usb_device_instance *device)
{
    unsigned int tmp;

    DBG("%s: begin\n", __FUNCTION__);

    /* Setup ep0 urb */
    if (!ep0_urb) {
	ep0_urb = usbd_alloc_urb (udc_device,
				  udc_device->bus->endpoint_array);
    }

    /* enable interrupts */
    OTG_USBINTR = USB_INTR_INT_EN | USB_INTR_ERR_INT_EN
		| USB_INTR_PTC_DETECT_EN | USB_INTR_RESET_EN
		| USB_INTR_DEVICE_SUSPEND | USB_INTR_SYS_ERR_EN;


    /* Set controller to Run */
    tmp = OTG_USBCMD;
    tmp |= USB_CMD_RUN_STOP;
    OTG_USBCMD = tmp;

    usbd_device_event_irq (device, DEVICE_HUB_CONFIGURED, 0);

    DBG("%s: complete\n", __FUNCTION__);
}

/* udc_disable
 *
 * disable the currently hooked device
 */
void udc_disable (void)
{
    unsigned int tmp;

    DBG("%s: begin\n", __FUNCTION__);

    /* disable all INTR */
    OTG_USBINTR = 0;

    /* Free ep0 URB */
    if (ep0_urb) {
	usbd_dealloc_urb(ep0_urb);
	ep0_urb = NULL;
    }

    /* set controller to Stop */
    tmp = OTG_USBCMD;
    tmp &= ~USB_CMD_RUN_STOP;
    OTG_USBCMD = tmp;

#if defined(CONFIG_PMIC)
    /* turn off charging, reenable autocharge */
    pmic_start_charging(pmic_charge_host, 1);
#endif
    
    DBG("%s: complete\n", __FUNCTION__);
}

/* udc_startup_events
 *
 * Enable the specified device
 */
void udc_startup_events (struct usb_device_instance *device)
{
#ifdef __DEBUG_UDC__
    print_debug = 1;
#endif

    DBG("%s: begin\n", __FUNCTION__);

    /* The DEVICE_INIT event puts the USB device in the state STATE_INIT. */
    usbd_device_event_irq (device, DEVICE_INIT, 0);

    /* The DEVICE_CREATE event puts the USB device in the state
     * STATE_ATTACHED.
     */
    usbd_device_event_irq (device, DEVICE_CREATE, 0);
    
    udc_device = device;
    
#if defined(CONFIG_PMIC)
    ichrg_value = 0;
#endif

    udc_enable(device);

#if defined(CONFIG_PMIC)
    /* initialize the LED state */
    pmic_enable_green_led(0);

    /* initialize autocharge state */
    fsl_arc_check_voltage();

#endif
    green_led = 0;

    DBG("%s: complete\n", __FUNCTION__);
}

/* udc_set_nak
 *
 * Allow upper layers to signal lower layers should not accept more RX data
 *
 */
void udc_set_nak (int epid)
{

}

/* udc_unset_nak
 *
 * Suspend sending of NAK tokens for DATA OUT tokens on a given endpoint.
 * Switch off NAKing on this endpoint to accept more data output from host.
 *
 */
void udc_unset_nak (int epid)
{

}


int udc_ep_set_halt(struct usb_device_instance *dev, unsigned char ep_index, int halt) {

    u32 tmp_epctrl = 0;
    int ep_num = (ep_index & USB_ENDPOINT_NUMBER_MASK);
    int dir = (ep_index & USB_DIR_IN);

    tmp_epctrl = OTG_EPCTL(ep_num);

    if (halt) {
	/* set the stall bit */
	if (dir)
	    tmp_epctrl |= EPCTRL_TX_EP_STALL;
	else
	    tmp_epctrl |= EPCTRL_RX_EP_STALL;
    } else {
	/* clear the stall bit and reset data toggle */
	if (dir) {
	    tmp_epctrl &= ~EPCTRL_TX_EP_STALL;
	    tmp_epctrl |= EPCTRL_TX_DATA_TOGGLE_RST;
	} else {
	    tmp_epctrl &= ~EPCTRL_RX_EP_STALL;
	    tmp_epctrl |= EPCTRL_RX_DATA_TOGGLE_RST;
	}
    }

    OTG_EPCTL(ep_num) = tmp_epctrl;

    usbd_endpoint_set_halt(dev, ep_index, halt); 

    return 0;
}
