/*
 * fsl_arc_udc.h
 *
 * Copyright (C) 2010 Amazon Technologies, Inc.  All rights reserved. 
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

#ifndef __FSL_ARC_UDC_H__
#define __FSL_ARC_UDC_H__

#define USB_MAX_ENDPOINTS		8
#define USB_MAX_PIPES			(USB_MAX_ENDPOINTS*2)
#define USB_MAX_CTRL_PAYLOAD		64
#define	USB_DR_SYS_OFFSET		0x400

#define USBID		__REG(USB_BASE_ADDR + 0x0000)
#define SBUSCFG		__REG(USB_BASE_ADDR + 0x0090)
#define OTG_DCCPARAMS	__REG(USB_BASE_ADDR + 0x0124)
#define OTG_USBCMD	__REG(USB_BASE_ADDR + 0x0140)
#define OTG_USBSTATUS	__REG(USB_BASE_ADDR + 0x0144)
#define OTG_USBINTR	__REG(USB_BASE_ADDR + 0x0148)
#define OTG_DEVICEADDR	__REG(USB_BASE_ADDR + 0x0154)
#define OTG_EPLISTADDR	__REG(USB_BASE_ADDR + 0x0158)
#define OTG_PORTSC1	__REG(USB_BASE_ADDR + 0x0184)
#define OTG_USBMODE	__REG(USB_BASE_ADDR + 0x01A8)
#define OTG_EPSETUPST	__REG(USB_BASE_ADDR + 0x01AC)
#define OTG_EPPRIME	__REG(USB_BASE_ADDR + 0x01B0)
#define OTG_EPFLUSH	__REG(USB_BASE_ADDR + 0x01B4)
#define OTG_EPSTAT	__REG(USB_BASE_ADDR + 0x01B8)
#define OTG_EPCOMPLETE	__REG(USB_BASE_ADDR + 0x01BC)
#define OTG_EPCTL(ep)	__REG(USB_BASE_ADDR + 0x01C0 + ((ep) * 0x04))

#define OTG_USBCTRL		__REG(USB_BASE_ADDR + 0x0800)
#define OTG_USB_MIRROR		__REG(USB_BASE_ADDR + 0x0804)
#define OTG_UTMI_PHY_CTRL0	__REG(USB_BASE_ADDR + 0x0808)
#define OTG_UTMI_PHY_CTRL1	__REG(USB_BASE_ADDR + 0x080C)


/* USB CMD  Register Bit Masks */
#define  USB_CMD_RUN_STOP                     (0x00000001)
#define  USB_CMD_CTRL_RESET                   (0x00000002)
#define  USB_CMD_PERIODIC_SCHEDULE_EN         (0x00000010)
#define  USB_CMD_ASYNC_SCHEDULE_EN            (0x00000020)
#define  USB_CMD_INT_AA_DOORBELL              (0x00000040)
#define  USB_CMD_ASP                          (0x00000300)
#define  USB_CMD_ASYNC_SCH_PARK_EN            (0x00000800)
#define  USB_CMD_SUTW                         (0x00002000)
#define  USB_CMD_ATDTW                        (0x00004000)
#define  USB_CMD_ITC                          (0x00FF0000)

/* bit 15,3,2 are frame list size */
#define  USB_CMD_FRAME_SIZE_1024              (0x00000000)
#define  USB_CMD_FRAME_SIZE_512               (0x00000004)
#define  USB_CMD_FRAME_SIZE_256               (0x00000008)
#define  USB_CMD_FRAME_SIZE_128               (0x0000000C)
#define  USB_CMD_FRAME_SIZE_64                (0x00008000)
#define  USB_CMD_FRAME_SIZE_32                (0x00008004)
#define  USB_CMD_FRAME_SIZE_16                (0x00008008)
#define  USB_CMD_FRAME_SIZE_8                 (0x0000800C)

/* bit 9-8 are async schedule park mode count */
#define  USB_CMD_ASP_00                       (0x00000000)
#define  USB_CMD_ASP_01                       (0x00000100)
#define  USB_CMD_ASP_10                       (0x00000200)
#define  USB_CMD_ASP_11                       (0x00000300)
#define  USB_CMD_ASP_BIT_POS                  (8)

/* bit 23-16 are interrupt threshold control */
#define  USB_CMD_ITC_NO_THRESHOLD             (0x00000000)
#define  USB_CMD_ITC_1_MICRO_FRM              (0x00010000)
#define  USB_CMD_ITC_2_MICRO_FRM              (0x00020000)
#define  USB_CMD_ITC_4_MICRO_FRM              (0x00040000)
#define  USB_CMD_ITC_8_MICRO_FRM              (0x00080000)
#define  USB_CMD_ITC_16_MICRO_FRM             (0x00100000)
#define  USB_CMD_ITC_32_MICRO_FRM             (0x00200000)
#define  USB_CMD_ITC_64_MICRO_FRM             (0x00400000)
#define  USB_CMD_ITC_BIT_POS                  (16)

/* USB STS Register Bit Masks */
#define  USB_STS_INT                          (0x00000001)
#define  USB_STS_ERR                          (0x00000002)
#define  USB_STS_PORT_CHANGE                  (0x00000004)
#define  USB_STS_FRM_LST_ROLL                 (0x00000008)
#define  USB_STS_SYS_ERR                      (0x00000010)
#define  USB_STS_IAA                          (0x00000020)
#define  USB_STS_RESET                        (0x00000040)
#define  USB_STS_SOF                          (0x00000080)
#define  USB_STS_SUSPEND                      (0x00000100)
#define  USB_STS_HC_HALTED                    (0x00001000)
#define  USB_STS_RCL                          (0x00002000)
#define  USB_STS_PERIODIC_SCHEDULE            (0x00004000)
#define  USB_STS_ASYNC_SCHEDULE               (0x00008000)

/* USB INTR Register Bit Masks */
#define  USB_INTR_INT_EN                      (0x00000001)
#define  USB_INTR_ERR_INT_EN                  (0x00000002)
#define  USB_INTR_PTC_DETECT_EN               (0x00000004)
#define  USB_INTR_FRM_LST_ROLL_EN             (0x00000008)
#define  USB_INTR_SYS_ERR_EN                  (0x00000010)
#define  USB_INTR_ASYN_ADV_EN                 (0x00000020)
#define  USB_INTR_RESET_EN                    (0x00000040)
#define  USB_INTR_SOF_EN                      (0x00000080)
#define  USB_INTR_DEVICE_SUSPEND              (0x00000100)
/* USB MODE Register Bit Masks */
#define  USB_MODE_CTRL_MODE_IDLE              (0x00000000)
#define  USB_MODE_CTRL_MODE_DEVICE            (0x00000002)
#define  USB_MODE_CTRL_MODE_HOST              (0x00000003)
#define  USB_MODE_CTRL_MODE_MASK              0x00000003
#define  USB_MODE_CTRL_MODE_RSV               (0x00000001)
#define  USB_MODE_ES                          0x00000004 /* (big) Endian Sel */
#define  USB_MODE_SETUP_LOCK_OFF              (0x00000008)
#define  USB_MODE_STREAM_DISABLE              (0x00000010)

/* Device Address bit masks */
#define  USB_DEVICE_ADDRESS_MASK              (0xFE000000)
#define  USB_DEVICE_ADDRESS_BIT_POS           (25)

/* Endpoint Setup Status bit masks */
#define  EP_SETUP_STATUS_MASK                 (0x0000003F)
#define  EP_SETUP_STATUS_EP0		      (0x00000001)

/* ENDPOINTCTRLx  Register Bit Masks */
#define  EPCTRL_TX_ENABLE                     (0x00800000)
#define  EPCTRL_TX_DATA_TOGGLE_RST            (0x00400000)	/* Not EP0 */
#define  EPCTRL_TX_DATA_TOGGLE_INH            (0x00200000)	/* Not EP0 */
#define  EPCTRL_TX_TYPE                       (0x000C0000)
#define  EPCTRL_TX_DATA_SOURCE                (0x00020000)	/* Not EP0 */
#define  EPCTRL_TX_EP_STALL                   (0x00010000)
#define  EPCTRL_RX_ENABLE                     (0x00000080)
#define  EPCTRL_RX_DATA_TOGGLE_RST            (0x00000040)	/* Not EP0 */
#define  EPCTRL_RX_DATA_TOGGLE_INH            (0x00000020)	/* Not EP0 */
#define  EPCTRL_RX_TYPE                       (0x0000000C)
#define  EPCTRL_RX_DATA_SINK                  (0x00000002)	/* Not EP0 */
#define  EPCTRL_RX_EP_STALL                   (0x00000001)

/* bit 19-18 and 3-2 are endpoint type */
#define  EPCTRL_EP_TYPE_CONTROL               (0)
#define  EPCTRL_EP_TYPE_ISO                   (1)
#define  EPCTRL_EP_TYPE_BULK                  (2)
#define  EPCTRL_EP_TYPE_INTERRUPT             (3)
#define  EPCTRL_TX_EP_TYPE_SHIFT              (18)
#define  EPCTRL_RX_EP_TYPE_SHIFT              (2)

/* endpoint list address bit masks */
#define USB_EP_LIST_ADDRESS_MASK              (0xfffff800)

/* PORTSCX  Register Bit Masks */
#define  PORTSCX_CURRENT_CONNECT_STATUS       (0x00000001)
#define  PORTSCX_CONNECT_STATUS_CHANGE        (0x00000002)
#define  PORTSCX_PORT_ENABLE                  (0x00000004)
#define  PORTSCX_PORT_EN_DIS_CHANGE           (0x00000008)
#define  PORTSCX_OVER_CURRENT_ACT             (0x00000010)
#define  PORTSCX_OVER_CURRENT_CHG             (0x00000020)
#define  PORTSCX_PORT_FORCE_RESUME            (0x00000040)
#define  PORTSCX_PORT_SUSPEND                 (0x00000080)
#define  PORTSCX_PORT_RESET                   (0x00000100)
#define  PORTSCX_PORT_HSP                     (0x00000200) 
#define  PORTSCX_LINE_STATUS_BITS             (0x00000C00)
#define  PORTSCX_PORT_POWER                   (0x00001000)
#define  PORTSCX_PORT_INDICTOR_CTRL           (0x0000C000)
#define  PORTSCX_PORT_TEST_CTRL               (0x000F0000)
#define  PORTSCX_WAKE_ON_CONNECT_EN           (0x00100000)
#define  PORTSCX_WAKE_ON_CONNECT_DIS          (0x00200000)
#define  PORTSCX_WAKE_ON_OVER_CURRENT         (0x00400000)
#define  PORTSCX_PHY_LOW_POWER_SPD            (0x00800000)
#define  PORTSCX_PORT_FORCE_FULL_SPEED        (0x01000000)
#define  PORTSCX_PORT_SPEED_MASK              (0x0C000000)
#define  PORTSCX_PORT_WIDTH                   (0x10000000)
#define  PORTSCX_PHY_TYPE_SEL                 (0xC0000000)

/* bit 11-10 are line status */
#define  PORTSCX_LINE_STATUS_SE0              (0x00000000)
#define  PORTSCX_LINE_STATUS_JSTATE           (0x00000400)
#define  PORTSCX_LINE_STATUS_KSTATE           (0x00000800)
#define  PORTSCX_LINE_STATUS_UNDEF            (0x00000C00)
#define  PORTSCX_LINE_STATUS_BIT_POS          (10)

/* bit 15-14 are port indicator control */
#define  PORTSCX_PIC_OFF                      (0x00000000)
#define  PORTSCX_PIC_AMBER                    (0x00004000)
#define  PORTSCX_PIC_GREEN                    (0x00008000)
#define  PORTSCX_PIC_UNDEF                    (0x0000C000)
#define  PORTSCX_PIC_BIT_POS                  (14)

/* bit 19-16 are port test control */
#define  PORTSCX_PTC_DISABLE                  (0x00000000)
#define  PORTSCX_PTC_JSTATE                   (0x00010000)
#define  PORTSCX_PTC_KSTATE                   (0x00020000)
#define  PORTSCX_PTC_SEQNAK                   (0x00030000)
#define  PORTSCX_PTC_PACKET                   (0x00040000)
#define  PORTSCX_PTC_FORCE_EN                 (0x00050000)
#define  PORTSCX_PTC_BIT_POS                  (16)

/* bit 27-26 are port speed */
#define  PORTSCX_PORT_SPEED_FULL              (0x00000000)
#define  PORTSCX_PORT_SPEED_LOW               (0x04000000)
#define  PORTSCX_PORT_SPEED_HIGH              (0x08000000)
#define  PORTSCX_PORT_SPEED_UNDEF             (0x0C000000)
#define  PORTSCX_SPEED_BIT_POS                (26)

/* bit 28 is parallel transceiver width for UTMI interface */
#define  PORTSCX_PTW                          (0x10000000)
#define  PORTSCX_PTW_8BIT                     (0x00000000)
#define  PORTSCX_PTW_16BIT                    (0x10000000)

/* bit 31-30 are port transceiver select */
#define  PORTSCX_PTS_UTMI                     (0x00000000)
#define  PORTSCX_PTS_ULPI                     (0x80000000)
#define  PORTSCX_PTS_FSLS                     (0xC0000000)
#define  PORTSCX_PTS_BIT_POS                  (30)

/* USBCTRL */
#define UCTRL_OWIR		(1 << 31)	/* OTG wakeup intr request received */
#define UCTRL_OSIC_MASK		(3 << 29)	/* OTG  Serial Interface Config: */
#define UCTRL_OSIC_DU6		(0 << 29)	/* Differential/unidirectional 6 wire */
#define UCTRL_OSIC_DB4		(1 << 29)	/* Differential/bidirectional  4 wire */
#define UCTRL_OSIC_SU6		(2 << 29)	/* single-ended/unidirectional 6 wire */
#define UCTRL_OSIC_SB3		(3 << 29)	/* single-ended/bidirectional  3 wire */

#define UCTRL_OUIE		(1 << 28)	/* OTG ULPI intr enable */
#define UCTRL_OWIE		(1 << 27)	/* OTG wakeup intr enable */
#define UCTRL_OBPVAL_RXDP	(1 << 26)	/* OTG RxDp status in bypass mode */
#define UCTRL_OBPVAL_RXDM	(1 << 25)	/* OTG RxDm status in bypass mode */
#define UCTRL_OPM		(1 << 24)	/* OTG power mask */
#define UCTRL_H2WIR		(1 << 23)	/* HOST2 wakeup intr request received */
#define UCTRL_H2SIC_MASK	(3 << 9)	/* HOST2 Serial Interface Config: */
#define UCTRL_H2SIC_DU6		(0 << 9)	/* Differential/unidirectional 6 wire */
#define UCTRL_H2SIC_DB4		(1 << 9)	/* Differential/bidirectional  4 wire */
#define UCTRL_H2SIC_SU6		(2 << 9)	/* single-ended/unidirectional 6 wire */
#define UCTRL_H2SIC_SB3		(3 << 9)	/* single-ended/bidirectional  3 wire */

#if defined(CONFIG_MX51)
#define UCTRL_H2UIE		(1 << 8)	/* HOST2 ULPI intr enable */
#define UCTRL_H2WIE		(1 << 7)	/* HOST2 wakeup intr enable */
#define UCTRL_H2PP		0	/* Power Polarity for uh2 */
#define UCTRL_H2PM		(1 << 4)	/* HOST2 power mask */
#define UCTRL_SER_DRV		(1 << 20)
#else
#define UCTRL_H2UIE		(1 << 20)	/* HOST2 ULPI intr enable */
#define UCTRL_H2WIE		(1 << 19)	/* HOST2 wakeup intr enable */
#define UCTRL_H2PP		(1 << 18)	/* Power Polarity for uh2 */
#define UCTRL_H2PM		(1 << 16)	/* HOST2 power mask */
#endif
#define UCTRL_H2OVBWK_EN	(1 << 6) /* OTG VBUS Wakeup Enable */
#define UCTRL_H2OIDWK_EN	(1 << 5) /* OTG ID Wakeup Enable */

#define UCTRL_H1WIR		(1 << 15)	/* HOST1 wakeup intr request received */
#define UCTRL_H1SIC_MASK	(3 << 13)	/* HOST1 Serial Interface Config: */
#define UCTRL_H1SIC_DU6		(0 << 13)	/* Differential/unidirectional 6 wire */
#define UCTRL_H1SIC_DB4		(1 << 13)	/* Differential/bidirectional  4 wire */
#define UCTRL_H1SIC_SU6		(2 << 13)	/* single-ended/unidirectional 6 wire */
#define UCTRL_H1SIC_SB3		(3 << 13)	/* single-ended/bidirectional  3 wire */
#define UCTRL_OLOCKD		(1 << 13)	/* otg lock disable */
#define UCTRL_H2LOCKD		(1 << 12)	/* HOST2 lock disable */
#define UCTRL_H1UIE		(1 << 12)	/* Host1 ULPI interrupt enable */

/* PHY control0 Register Bit Masks */
#define USB_UTMI_PHYCTRL_CONF2	(1 << 26)
#define USB_UTMI_PHYCTRL_UTMI_ENABLE (1 << 24)
#define USB_UTMI_PHYCTRL_CHGRDETEN (1 << 24)    /* Enable Charger Detector */
#define USB_UTMI_PHYCTRL_CHGRDETON (1 << 23)    /* Charger Detector Power On Control */
#define USB_UTMI_PHYCTRL_OC_POL	(1 << 9)	/* OTG Polarity of Overcurrent */
#define USB_UTMI_PHYCTRL_OC_DIS	(1 << 8)	/* OTG Disable Overcurrent Event */
#define USB_UH1_OC_DIS	(1 << 5)		/* UH1 Disable Overcurrent Event */

/* PHY control1 Register Bit Masks */
#define USB_UTMI_PHYCTRL2_PLLDIV_MASK		0x3
#define USB_UTMI_PHYCTRL2_PLLDIV_SHIFT		0
#define USB_UTMI_PHYCTRL2_HSDEVSEL_MASK		0x3
#define USB_UTMI_PHYCTRL2_HSDEVSEL_SHIFT	19

/*!
 * Endpoint Queue Head data struct
 * Rem: all the variables of qh are LittleEndian Mode
 * and NEXT_POINTER_MASK should operate on a LittleEndian, Phy Addr
 */
struct ep_queue_head {
	/*!
	 * Mult(31-30) , Zlt(29) , Max Pkt len  and IOS(15)
	 */
	u32 max_pkt_length;

	/*!
	 *  Current dTD Pointer(31-5)
	 */
	u32 curr_dtd_ptr;

	/*!
	 *  Next dTD Pointer(31-5), T(0)
	 */
	u32 next_dtd_ptr;

	/*!
	 *  Total bytes (30-16), IOC (15), MultO(11-10), STS (7-0)
	 */
	u32 size_ioc_int_sts;

	/*!
	 * Buffer pointer Page 0 (31-12)
	 */
	u32 buff_ptr0;

	/*!
	 * Buffer pointer Page 1 (31-12)
	 */
	u32 buff_ptr1;

	/*!
	 * Buffer pointer Page 2 (31-12)
	 */
	u32 buff_ptr2;

	/*!
	 * Buffer pointer Page 3 (31-12)
	 */
	u32 buff_ptr3;

	/*!
	 * Buffer pointer Page 4 (31-12)
	 */
	u32 buff_ptr4;

	/*!
	 * reserved field 1
	 */
	u32 res1;
	/*!
	 * Setup data 8 bytes
	 */
	u8 setup_buffer[8];	/* Setup data 8 bytes */

	/*!
	 * reserved field 2,pad out to 64 bytes
	 */
	u32 res2[4];
} __attribute__ ((packed));

/* Endpoint Queue Head Bit Masks */
#define  EP_QUEUE_HEAD_MULT_POS               (30)
#define  EP_QUEUE_HEAD_ZLT_SEL                (0x20000000)
#define  EP_QUEUE_HEAD_MAX_PKT_LEN_POS        (16)
#define  EP_QUEUE_HEAD_MAX_PKT_LEN(ep_info)   (((ep_info)>>16)&0x07ff)
#define  EP_QUEUE_HEAD_IOS                    (0x00008000)
#define  EP_QUEUE_HEAD_NEXT_TERMINATE         (0x00000001)
#define  EP_QUEUE_HEAD_IOC                    (0x00008000)
#define  EP_QUEUE_HEAD_MULTO                  (0x00000C00)
#define  EP_QUEUE_HEAD_STATUS_HALT	      (0x00000040)
#define  EP_QUEUE_HEAD_STATUS_ACTIVE          (0x00000080)
#define  EP_QUEUE_CURRENT_OFFSET_MASK         (0x00000FFF)
#define  EP_QUEUE_HEAD_NEXT_POINTER_MASK      0xFFFFFFE0
#define  EP_QUEUE_FRINDEX_MASK                (0x000007FF)
#define  EP_MAX_LENGTH_TRANSFER               (0x4000)

/*!
 * Endpoint Transfer Descriptor data struct
 *  Rem: all the variables of td are LittleEndian Mode
 *       must be 32-byte aligned
 */
struct ep_td_struct {
	/*!
	 *  Next TD pointer(31-5), T(0) set indicate invalid
	 */
	u32 next_td_ptr;

	/*!
	 *  Total bytes (30-16), IOC (15),MultO(11-10), STS (7-0)
	 */
	u32 size_ioc_sts;

	/*!
	 * Buffer pointer Page 0
	 */
	u32 buff_ptr0;

	/*!
	 * Buffer pointer Page 1
	 */
	u32 buff_ptr1;

	/*!
	 * Buffer pointer Page 2
	 */
	u32 buff_ptr2;

	/*!
	 * Buffer pointer Page 3
	 */
	u32 buff_ptr3;

	/*!
	 * Buffer pointer Page 4
	 */
	u32 buff_ptr4;

	/*!
	 * virtual address of next td
	 * */
	struct ep_td_struct *next_td_virt;

	/*!
	 * make it an even 16 words
	 * */
	u32 res[8];
}  __attribute__ ((packed));

/*!
 * Endpoint Transfer Descriptor bit Masks
 */
#define  DTD_NEXT_TERMINATE                   (0x00000001)
#define  DTD_IOC                              (0x00008000)
#define  DTD_STATUS_ACTIVE                    (0x00000080)
#define  DTD_STATUS_HALTED                    (0x00000040)
#define  DTD_STATUS_DATA_BUFF_ERR             (0x00000020)
#define  DTD_STATUS_TRANSACTION_ERR           (0x00000008)
#define  DTD_RESERVED_FIELDS                  (0x80007300)
#define  DTD_ADDR_MASK                        0xFFFFFFE0
#define  DTD_PACKET_SIZE                      (0x7FFF0000)
#define  DTD_LENGTH_BIT_POS                   (16)
#define  DTD_ERROR_MASK                       (DTD_STATUS_HALTED | \
                                               DTD_STATUS_DATA_BUFF_ERR | \
                                               DTD_STATUS_TRANSACTION_ERR)
/* Alignment requirements; must be a power of two */
#define DTD_ALIGNMENT				0x20
#define QH_ALIGNMENT				2048

/* Controller dma boundary */
#define UDC_DMA_BOUNDARY			0x1000

/*
 * ### pipe direction macro from device view
 */
#define USB_RECV	(0)	/* OUT EP */
#define USB_SEND	(1)	/* IN EP */

#endif /* __FSL_ARC_UDC_H__ */
