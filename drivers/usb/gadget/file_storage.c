/*
 * file_storage.c
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

/* Fake file storage gadget for charging.  Calling file_storage_enable()
 * causes the device to enumerate on the host so we can draw 500 mA for
 * charging.  
 */

#include <common.h>
#include <usbdevice.h>
#include <usb_defs.h>
#include <usbdescriptors.h>

#if defined(CONFIG_PMIC)
#include <pmic.h>
#endif

#define EP0_MAX_PACKET_SIZE 64
#define UDC_OUT_ENDPOINT 1
#define UDC_IN_ENDPOINT 1

#define NUM_CONFIGS    1
#define MAX_INTERFACES 1
#define NUM_INTERFACES MAX_INTERFACES
#define NUM_ENDPOINTS  2

#define EP_OUT		1
#define EP_IN		2

#define STR_LANG		0x00
#define STR_MANUFACTURER	0x01
#define STR_PRODUCT		0x02
#define STR_SERIAL		0x03
#define STR_COUNT		0x04

#define FILE_STORAGE_MAXPOWER 0xFA

/* USB protocol value = the transport method */
#define USB_PR_CBI	0x00		// Control/Bulk/Interrupt
#define USB_PR_CB	0x01		// Control/Bulk w/o interrupt
#define USB_PR_BULK	0x50		// Bulk-only

/* USB subclass value = the protocol encapsulation */
#define USB_SC_RBC	0x01		// Reduced Block Commands (flash)
#define USB_SC_8020	0x02		// SFF-8020i, MMC-2, ATAPI (CD-ROM)
#define USB_SC_QIC	0x03		// QIC-157 (tape)
#define USB_SC_UFI	0x04		// UFI (floppy)
#define USB_SC_8070	0x05		// SFF-8070i (removable)
#define USB_SC_SCSI	0x06		// Transparent SCSI

#define FILE_STORAGE_RX_MAX	16384

/* Bulk-only class specific requests */
#define USB_BULK_RESET_REQUEST		0xff
#define USB_BULK_GET_MAX_LUN_REQUEST	0xfe

/* SCSI commands that we recognize */
#define SC_TEST_UNIT_READY		0x00
#define SC_REQUEST_SENSE		0x03
#define SC_READ_6			0x08
#define SC_INQUIRY			0x12
#define SC_MODE_SENSE_6			0x1a
#define SC_PREVENT_ALLOW_MEDIUM_REMOVAL	0x1e
#define SC_READ_FORMAT_CAPACITIES	0x23
#define SC_READ_CAPACITY		0x25
#define SC_READ_10			0x28
#define SC_MODE_SENSE_10		0x5a
#define SC_READ_12			0xa8

/* Command Status Wrapper */
struct bulk_cs_wrap {
	__le32	Signature;		// Should = 'USBS'
	u32	Tag;			// Same as original command
	__le32	Residue;		// Amount not transferred
	u8	Status;			// See below
};

#define USB_BULK_CS_WRAP_LEN	13
#define USB_BULK_CS_SIG		0x53425355	// Spells out 'USBS'
#define USB_STATUS_PASS		0
#define USB_STATUS_FAIL		1
#define USB_STATUS_PHASE_ERROR	2

#define ERR(fmt, args...)\
	serial_printf("ERROR : [%s] %s:%d: "fmt,\
				__FILE__,__FUNCTION__,__LINE__, ##args)
//#define __DEBUG_FILE_STORAGE__ 1
#ifdef __DEBUG_FILE_STORAGE__
#define DBG(fmt,args...)\
		serial_printf("[%s] %s:%d: "fmt,\
				__FILE__,__FUNCTION__,__LINE__, ##args)
#else
#define DBG(fmt,args...)
#endif

/*
 * Serial number
 */
#define SERIAL_NUM_LEN 16

static char serial_number[SERIAL_NUM_LEN + 1];

extern const char *version_string;

/*
 * Instance variables
 */
static int file_storage_configured = 0;
static int file_storage_exit = 0;

static int file_storage_setup_handler(struct usb_device_request *request, 
				      struct urb *urb);

static struct usb_device_instance device_instance[1];
static struct usb_bus_instance bus_instance[1];
static struct usb_configuration_instance hs_config_instance[NUM_CONFIGS];
static struct usb_configuration_instance fs_config_instance[NUM_CONFIGS];
static struct usb_interface_instance hs_interface_instance[MAX_INTERFACES];
static struct usb_interface_instance fs_interface_instance[MAX_INTERFACES];
static struct usb_alternate_instance hs_alternate_instance[MAX_INTERFACES];
static struct usb_alternate_instance fs_alternate_instance[MAX_INTERFACES];
/* one extra for control endpoint */
static struct usb_endpoint_instance endpoint_instance[NUM_ENDPOINTS+1];

static struct usb_string_descriptor *file_storage_string_table[STR_COUNT];

/* USB Descriptor Strings */
static u8 wstrLang[4] = {4,USB_DT_STRING,0x9,0x4};
static u8 wstrManufacturer[2 + 2*(sizeof(CONFIG_USBD_MANUFACTURER)-1)];
static u8 wstrProduct[2 + 2*(sizeof(CONFIG_USBD_PRODUCT_NAME)-1)];
static u8 wstrSerial[2 + (2*SERIAL_NUM_LEN)];

/* defined and used by gadget/ep0.c */
extern struct usb_string_descriptor **usb_strings;

/* Standard USB Data Structures */
static struct usb_interface_descriptor hs_interface_descriptors[MAX_INTERFACES];
static struct usb_interface_descriptor fs_interface_descriptors[MAX_INTERFACES];
static struct usb_endpoint_descriptor *hs_ep_descriptor_ptrs[NUM_ENDPOINTS];
static struct usb_endpoint_descriptor *fs_ep_descriptor_ptrs[NUM_ENDPOINTS];
static struct usb_configuration_descriptor *hs_configuration_descriptor = 0;
static struct usb_configuration_descriptor *fs_configuration_descriptor = 0;
static struct usb_device_descriptor device_descriptor = {
	.bLength = sizeof(struct usb_device_descriptor),
	.bDescriptorType =	USB_DT_DEVICE,
	.bcdUSB =		cpu_to_le16(USB_BCD_VERSION),
	.bDeviceClass =		0x00,
	.bDeviceSubClass =	0x00,
	.bDeviceProtocol =	0x00,
	.bMaxPacketSize0 =	EP0_MAX_PACKET_SIZE,
	.idVendor =		cpu_to_le16(CONFIG_USBD_VENDORID),
	.idProduct =		cpu_to_le16(CONFIG_USBD_PRODUCTID_FILE_STORAGE),
	.bcdDevice =		cpu_to_le16(0x1),
	.iManufacturer =	STR_MANUFACTURER,
	.iProduct =		STR_PRODUCT,
	.iSerialNumber =	STR_SERIAL,
	.bNumConfigurations =	NUM_CONFIGS
};

static struct usb_qualifier_descriptor qualifier_descriptor = {
    .bLength = sizeof(struct usb_qualifier_descriptor),
    .bDescriptorType = USB_DT_DEVICE_QUALIFIER,
    .bcdUSB = __constant_cpu_to_le16(0x0200),
    .bDeviceClass = USB_CLASS_PER_INTERFACE,
    .bMaxPacketSize0 = EP0_MAX_PACKET_SIZE,
    .bNumConfigurations = NUM_CONFIGS,
};

struct file_storage_config_desc {

    struct usb_configuration_descriptor configuration_desc;
    struct usb_interface_descriptor interface_desc[NUM_INTERFACES];
    struct usb_endpoint_descriptor data_endpoints[NUM_ENDPOINTS];

} __attribute__((packed));

static struct file_storage_config_desc file_storage_hs_configuration_descriptors[NUM_CONFIGS] ={
    {
	.configuration_desc ={
	    .bLength = sizeof(struct usb_configuration_descriptor),
	    .bDescriptorType = USB_DT_CONFIG,
	    .wTotalLength = cpu_to_le16(sizeof(struct file_storage_config_desc)),
	    .bNumInterfaces = NUM_INTERFACES,
	    .bConfigurationValue = 1,
	    .bmAttributes = BMATTRIBUTE_SELF_POWERED|BMATTRIBUTE_RESERVED,
	    .bMaxPower = FILE_STORAGE_MAXPOWER
	},
	.interface_desc = {
	     {
		 .bLength  =
		 sizeof(struct usb_interface_descriptor),
		 .bDescriptorType = USB_DT_INTERFACE,
		 .bInterfaceNumber = 0,
		 .bAlternateSetting = 0,
		 .bNumEndpoints = NUM_ENDPOINTS,
		 .bInterfaceClass = USB_CLASS_MASS_STORAGE,
		 .bInterfaceSubClass = USB_SC_SCSI,
		 .bInterfaceProtocol = USB_PR_BULK,
	     },
	 },
	.data_endpoints  = {
	     {
		 .bLength = sizeof(struct usb_endpoint_descriptor),
		 .bDescriptorType = USB_DT_ENDPOINT,
		 .bEndpointAddress = UDC_OUT_ENDPOINT | USB_DIR_OUT,
		 .bmAttributes = USB_ENDPOINT_XFER_BULK,
		 .wMaxPacketSize = cpu_to_le16(512),
	     },
	     {
		 .bLength = sizeof(struct usb_endpoint_descriptor),
		 .bDescriptorType = USB_DT_ENDPOINT,
		 .bEndpointAddress = UDC_IN_ENDPOINT | USB_DIR_IN,
		 .bmAttributes = USB_ENDPOINT_XFER_BULK,
		 .wMaxPacketSize = cpu_to_le16(512),
	     },
	 },
    },
};

static struct file_storage_config_desc file_storage_fs_configuration_descriptors[NUM_CONFIGS] ={
    {
	.configuration_desc ={
	    .bLength = sizeof(struct usb_configuration_descriptor),
	    .bDescriptorType = USB_DT_CONFIG,
	    .wTotalLength = cpu_to_le16(sizeof(struct file_storage_config_desc)),
	    .bNumInterfaces = NUM_INTERFACES,
	    .bConfigurationValue = 1,
	    .bmAttributes = BMATTRIBUTE_SELF_POWERED|BMATTRIBUTE_RESERVED,
	    .bMaxPower = 0x32
	},
	.interface_desc = {
	     {
		 .bLength  =
		 sizeof(struct usb_interface_descriptor),
		 .bDescriptorType = USB_DT_INTERFACE,
		 .bInterfaceNumber = 0,
		 .bAlternateSetting = 0,
		 .bNumEndpoints = NUM_ENDPOINTS,
		 .bInterfaceClass = USB_CLASS_MASS_STORAGE,
		 .bInterfaceSubClass = USB_SC_SCSI,
		 .bInterfaceProtocol = USB_PR_BULK,
	     },
	 },
	.data_endpoints  = {
	     {
		 .bLength = sizeof(struct usb_endpoint_descriptor),
		 .bDescriptorType = USB_DT_ENDPOINT,
		 .bEndpointAddress = UDC_OUT_ENDPOINT | USB_DIR_OUT,
		 .bmAttributes = USB_ENDPOINT_XFER_BULK,
		 .wMaxPacketSize = cpu_to_le16(64),
	     },
	     {
		 .bLength = sizeof(struct usb_endpoint_descriptor),
		 .bDescriptorType = USB_DT_ENDPOINT,
		 .bEndpointAddress = UDC_IN_ENDPOINT | USB_DIR_IN,
		 .bmAttributes = USB_ENDPOINT_XFER_BULK,
		 .wMaxPacketSize = cpu_to_le16(64),
	     },
	 },
    },
};

static void file_storage_init_endpoints (struct usb_device_instance *device)
{
    int i;
    struct usb_endpoint_descriptor **epdesc;

    bus_instance->max_endpoints = NUM_ENDPOINTS + 1;

    if (device->speed == USB_SPEED_HIGH) {
	DBG("using HIGH speed descriptors\n");
	epdesc = hs_ep_descriptor_ptrs;
    } else {
	DBG("using FULL speed descriptors\n");
	epdesc = fs_ep_descriptor_ptrs;
    }

    for (i = 1; i <= NUM_ENDPOINTS; i++) {
	memset (&endpoint_instance[i], 0,
		sizeof (struct usb_endpoint_instance));

	endpoint_instance[i].endpoint_address =
	    epdesc[i - 1]->bEndpointAddress;

	endpoint_instance[i].rcv_attributes =
	    epdesc[i - 1]->bmAttributes;

	endpoint_instance[i].rcv_packetSize =
	    le16_to_cpu(epdesc[i - 1]->wMaxPacketSize);

	endpoint_instance[i].tx_attributes =
	    epdesc[i - 1]->bmAttributes;

	endpoint_instance[i].tx_packetSize =
	    le16_to_cpu(epdesc[i - 1]->wMaxPacketSize);

	endpoint_instance[i].tx_attributes =
	    epdesc[i - 1]->bmAttributes;

	urb_link_init (&endpoint_instance[i].rcv);
	urb_link_init (&endpoint_instance[i].rdy);
	urb_link_init (&endpoint_instance[i].tx);
	urb_link_init (&endpoint_instance[i].done);

	if (endpoint_instance[i].endpoint_address & USB_DIR_IN)
	    endpoint_instance[i].tx_urb =
		usbd_alloc_urb (device,
				&endpoint_instance[i]);
	else {
	    endpoint_instance[i].rcv_urb =
		usbd_alloc_urb (device,
				&endpoint_instance[i]);
	}

	udc_setup_ep (device, i, &endpoint_instance[i]);
    }

}

static void file_storage_event_handler (struct usb_device_instance *device,
				    usb_device_event_t event, int data)
{
    switch (event) {
      case DEVICE_RESET:
      case DEVICE_BUS_INACTIVE:
      case DEVICE_DE_CONFIGURED:

#if 0 // BEN TODO - do we want to exit?
	// if the cable is unplugged after configuring, exit
	if (file_storage_configured)
	    file_storage_exit = 1;
#endif

	file_storage_configured = 0;
	break;

      case DEVICE_ADDRESS_ASSIGNED:
	break;

      case DEVICE_CONFIGURED:
	{
	    struct usb_endpoint_instance *endpoint = &endpoint_instance[EP_OUT];
    
	    file_storage_configured = 1;
	    file_storage_init_endpoints (device);
	    
	    endpoint->rcv_urb->buffer = (u8 *) endpoint->rcv_urb->buffer_data;
	    endpoint->rcv_urb->buffer_length = FILE_STORAGE_RX_MAX;
	    endpoint->rcv_urb->status = 0;
	    endpoint->rcv_urb->actual_length = 0;

	    udc_endpoint_read(endpoint);
	}
	break;

      default:
	break;
    }
}

/* utility function for converting char* to wide string used by USB */
static void str2wide (char *str, u16 * wide)
{
	int i;
	for (i = 0; i < strlen (str) && str[i]; i++){
		#if defined(__LITTLE_ENDIAN)
			wide[i] = (u16) str[i];
		#elif defined(__BIG_ENDIAN)
			wide[i] = ((u16)(str[i])<<8);
		#else
			#error "__LITTLE_ENDIAN or __BIG_ENDIAN undefined"
		#endif
	}
}

static void file_storage_init_strings (void)
{
    struct usb_string_descriptor *string;

    file_storage_string_table[STR_LANG] =
	(struct usb_string_descriptor*)wstrLang;

    string = (struct usb_string_descriptor *) wstrManufacturer;
    string->bLength = sizeof(wstrManufacturer);
    string->bDescriptorType = USB_DT_STRING;
    str2wide (CONFIG_USBD_MANUFACTURER, string->wData);
    file_storage_string_table[STR_MANUFACTURER]=string;


    string = (struct usb_string_descriptor *) wstrProduct;
    string->bLength = sizeof(wstrProduct);
    string->bDescriptorType = USB_DT_STRING;
    str2wide (CONFIG_USBD_PRODUCT_NAME, string->wData);
    file_storage_string_table[STR_PRODUCT]=string;

    string = (struct usb_string_descriptor *) wstrSerial;
    string->bLength = sizeof(wstrSerial);
    string->bDescriptorType = USB_DT_STRING;
    str2wide (serial_number, string->wData);
    file_storage_string_table[STR_SERIAL]=string;

    /* Now, initialize the string table for ep0 handling */
    usb_strings = file_storage_string_table;
}

static void file_storage_init_instances (void)
{
    int i;

    /* initialize device instance */
    memset (device_instance, 0, sizeof (struct usb_device_instance));
    device_instance->device_state = STATE_INIT;
    device_instance->device_descriptor = &device_descriptor;
    device_instance->qualifier_descriptor = &qualifier_descriptor;
    device_instance->event = file_storage_event_handler;
    device_instance->cdc_recv_setup = file_storage_setup_handler;
    device_instance->bus = bus_instance;
    device_instance->configurations = NUM_CONFIGS;
    device_instance->hs_configuration_instance_array = hs_config_instance;
    device_instance->fs_configuration_instance_array = fs_config_instance;

    /* initialize bus instance */
    memset (bus_instance, 0, sizeof (struct usb_bus_instance));
    bus_instance->device = device_instance;
    bus_instance->endpoint_array = endpoint_instance;
    bus_instance->max_endpoints = 1;
    bus_instance->maxpacketsize = EP0_MAX_PACKET_SIZE;

    /* Configuration Descriptor */
    hs_configuration_descriptor = (struct usb_configuration_descriptor*)
	&file_storage_hs_configuration_descriptors;
    fs_configuration_descriptor = (struct usb_configuration_descriptor*)
	&file_storage_fs_configuration_descriptors;

    /* configuration instance */
    memset (hs_config_instance, 0,
	    sizeof (struct usb_configuration_instance));
    hs_config_instance->interfaces = NUM_INTERFACES;
    hs_config_instance->configuration_descriptor = hs_configuration_descriptor;
    hs_config_instance->interface_instance_array = hs_interface_instance;
    memset (fs_config_instance, 0,
	    sizeof (struct usb_configuration_instance));
    fs_config_instance->interfaces = NUM_INTERFACES;
    fs_config_instance->configuration_descriptor = fs_configuration_descriptor;
    fs_config_instance->interface_instance_array = fs_interface_instance;

    /* interface instance */
    memset (hs_interface_instance, 0,
	    sizeof (struct usb_interface_instance));
    hs_interface_instance->alternates = 1;
    hs_interface_instance->alternates_instance_array = hs_alternate_instance;
    memset (fs_interface_instance, 0,
	    sizeof (struct usb_interface_instance));
    fs_interface_instance->alternates = 1;
    fs_interface_instance->alternates_instance_array = fs_alternate_instance;

    /* alternates instance */
    memset (hs_alternate_instance, 0,
	    sizeof (struct usb_alternate_instance));
    hs_alternate_instance->interface_descriptor = hs_interface_descriptors;
    hs_alternate_instance->endpoints = NUM_ENDPOINTS;
    hs_alternate_instance->endpoints_descriptor_array = hs_ep_descriptor_ptrs;
    memset (fs_alternate_instance, 0,
	    sizeof (struct usb_alternate_instance));
    fs_alternate_instance->interface_descriptor = fs_interface_descriptors;
    fs_alternate_instance->endpoints = NUM_ENDPOINTS;
    fs_alternate_instance->endpoints_descriptor_array = fs_ep_descriptor_ptrs;

    /* Assign endpoint descriptors */
    hs_ep_descriptor_ptrs[0] = &file_storage_hs_configuration_descriptors[0].data_endpoints[0];
    hs_ep_descriptor_ptrs[1] = &file_storage_hs_configuration_descriptors[0].data_endpoints[1];
    fs_ep_descriptor_ptrs[0] = &file_storage_fs_configuration_descriptors[0].data_endpoints[0];
    fs_ep_descriptor_ptrs[1] = &file_storage_fs_configuration_descriptors[0].data_endpoints[1];

    /* endpoint instances */
    memset (&endpoint_instance[0], 0,
	    sizeof (struct usb_endpoint_instance));
    endpoint_instance[0].endpoint_address = 0;
    endpoint_instance[0].rcv_packetSize = EP0_MAX_PACKET_SIZE;
    endpoint_instance[0].rcv_attributes = USB_ENDPOINT_XFER_CONTROL;
    endpoint_instance[0].tx_packetSize = EP0_MAX_PACKET_SIZE;
    endpoint_instance[0].tx_attributes = USB_ENDPOINT_XFER_CONTROL;
    udc_setup_ep (device_instance, 0, &endpoint_instance[0]);

    /* non-control eps will get populated after speed determination */
    for (i = 1; i <= NUM_ENDPOINTS; i++) {
	memset (&endpoint_instance[i], 0,
		sizeof (struct usb_endpoint_instance));
    }
}

static int file_storage_setup_handler(struct usb_device_request *request, 
				      struct urb *urb)
{
    unsigned char lun;

    DBG("begin request=0x%x\n", request->bRequest);

    switch (request->bRequest) {
      case USB_BULK_GET_MAX_LUN_REQUEST:
	   
	if (request->bmRequestType != (USB_DIR_IN |
				   USB_TYPE_CLASS | USB_RECIP_INTERFACE))
		break;

	if (request->wIndex != 0 || request->wValue != 0) {
	    break;
	}
	
	DBG("send max lun\n");

	lun = 0;
	memcpy (urb->buffer , &lun, sizeof(lun));
	urb->actual_length = sizeof(lun);

	return 0;

      case USB_BULK_RESET_REQUEST:

	if (request->bmRequestType != (USB_DIR_OUT |
				   USB_TYPE_CLASS | USB_RECIP_INTERFACE))
		break;

	if (request->wIndex != 0 || request->wValue != 0) {
	    break;
	}
	
	// do nothing, just send ACK
	urb->actual_length = 0;

	return 0;
    }

    return -1;
}

static int file_storage_wait_for_send(struct usb_endpoint_instance *endpoint, 
					       struct urb *urb) 
{
    endpoint->sent = 0;
    endpoint->last = urb->actual_length;

    if (udc_endpoint_write (endpoint)) {
	/* Write pre-empted by RX */
	ERR("ep write fail!\n");
	return -1;
    }
	    
    // wait for complete irq
    // make sure we at least wait for 1 interrupt if ZLP
    do {
	udc_irq();
    } while (endpoint->sent < urb->actual_length && urb->status == 0); 

    if (urb->status != 0) {
	ERR("Couldn't send reply packet: 0x%x\n", urb->status);
	return -1;
    }   

    return 0;
}

static int file_storage_send_reply(char reply_valid, 
				   unsigned char *reply, unsigned int replylen, 
				   unsigned int tag, unsigned char status) {
    
    struct usb_endpoint_instance *endpoint = &endpoint_instance[EP_IN];
    struct urb *current_urb = NULL;
    struct bulk_cs_wrap *status_reply;
    int ret;

    current_urb = endpoint->tx_urb;

    DBG("begin.  replylen=%d status=%d\n", replylen, status);

    /* send reply */
    if (reply_valid) {

	int space_avail;
	int popnum;
	int total = 0;

	DBG("sending reply\n");

	/* Break buffer into urb sized pieces,
	 * and link each to the endpoint
	 */
	do {

	    if (!current_urb) {
		printf ("%s: current_urb is NULL, length %d\n", __FUNCTION__, replylen);
		return total;
	    }
	    
	    current_urb->buffer = (unsigned char *) current_urb->buffer_data;
	    current_urb->buffer_length = (URB_BUF_SIZE * sizeof(current_urb->buffer_data[0]));
	    current_urb->status = 0;

	    space_avail = current_urb->buffer_length;
	    popnum = MIN (space_avail, replylen);

	    if (popnum)
		memcpy(current_urb->buffer + total, reply + total, popnum);

	    current_urb->actual_length = popnum;
	    total += popnum;
	    replylen -= popnum;

	    DBG("sending %d bytes\n", current_urb->actual_length);

	    ret = file_storage_wait_for_send(endpoint, current_urb);
	    if (ret)
		return ret;

	    DBG("send reply complete\n");

	} while (replylen > 0);
    }

    DBG("send status\n");

    /* send status */
    current_urb->actual_length = USB_BULK_CS_WRAP_LEN;
    current_urb->buffer = (unsigned char *) current_urb->buffer_data;
    current_urb->buffer_length = (URB_BUF_SIZE * sizeof(current_urb->buffer_data[0]));
    current_urb->status = 0;

    status_reply = (struct bulk_cs_wrap *) current_urb->buffer;

    status_reply->Signature =  __constant_cpu_to_le32(USB_BULK_CS_SIG);
    status_reply->Tag = tag;
    status_reply->Residue = 0;
    status_reply->Status = status;

    ret = file_storage_wait_for_send(endpoint, current_urb);
    if (ret)
	return ret;

    DBG("finish\n");

    return 0;
}

static void file_storage_parse_cmd(struct usb_endpoint_instance *endpoint, unsigned char *cmdbuf) 
{
    unsigned char reply[255];
    unsigned char replylen = cmdbuf[19];
    unsigned int cmd = cmdbuf[15];
    unsigned int cmdtag;

    DBG("cmd=0x%x replylen=%d\n", cmd, replylen);

    // copy 4 byte cmdtag
    memcpy(&cmdtag, cmdbuf + 4, sizeof(cmdtag));

    switch (cmd) {

      case SC_INQUIRY:

	memset(reply, 0, replylen);

	reply[1] = 0x80;        // Removable
	reply[2] = 2;		// ANSI SCSI level 2
	reply[3] = 2;		// SCSI-2 INQUIRY data format
	reply[4] = replylen - 5;	// Additional length

	sprintf((char *) (reply + 8), "%-8s%-16s%04x", 
		CONFIG_USBD_MANUFACTURER, 
		CONFIG_USBD_PRODUCT_NAME, 
		0x0100);

	/* send reply */
	file_storage_send_reply(1, reply, replylen, cmdtag, USB_STATUS_PASS);

	break;

      case SC_REQUEST_SENSE:

	memset(reply, 0, replylen);

	/* hardcode sense reply to be 'SS_MEDIUM_NOT_PRESENT' */
	reply[0] = 0x70;
	reply[7] = 10;	// command length

	reply[2] = 0x02;  // sense key
	reply[12] = 0x3A; // sense code
	reply[13] = 0;	// sense code qualifier

	file_storage_send_reply(1, reply, replylen, cmdtag, USB_STATUS_PASS);
	
	break;

      case SC_PREVENT_ALLOW_MEDIUM_REMOVAL:

	/* just acknowledge the command */
	file_storage_send_reply(0, NULL, 0, cmdtag, USB_STATUS_PASS);
	
	break;

      case SC_MODE_SENSE_6:
      case SC_READ_6:
      case SC_READ_10:
      case SC_READ_12:
      case SC_READ_CAPACITY:
      case SC_READ_FORMAT_CAPACITIES:

	/* send ZLP with fail status otherwise windows gets confused */
	DBG("Send ZLP\n");
	file_storage_send_reply(1, NULL, 0, cmdtag, USB_STATUS_FAIL);

	break;
	
      case SC_TEST_UNIT_READY:

	/* always send fail reply */
	file_storage_send_reply(0, NULL, 0, cmdtag, USB_STATUS_FAIL);
	
	break;

      default:

	ERR("Unknown command: 0x%02x\n", cmd);
	file_storage_send_reply(0, NULL, 0, cmdtag, USB_STATUS_FAIL);

	break;
    }

    // requeue command buffer
    endpoint->rcv_urb->buffer = (u8 *) endpoint->rcv_urb->buffer_data;
    endpoint->rcv_urb->buffer_length = FILE_STORAGE_RX_MAX;  
    endpoint->rcv_urb->status = 0;
    endpoint->rcv_urb->actual_length = 0;

    udc_endpoint_read(endpoint);
}

static int file_storage_check_for_data(void) {

    struct usb_endpoint_instance *endpoint =
	&endpoint_instance[EP_OUT];

    if (endpoint->rcv_urb->status) {

	ERR("error receiving usb packet: 0x%x\n", endpoint->rcv_urb->status);

	// requeue command buffer
	endpoint->rcv_urb->buffer = (u8 *) endpoint->rcv_urb->buffer_data;
	endpoint->rcv_urb->buffer_length = FILE_STORAGE_RX_MAX;  
	endpoint->rcv_urb->status = 0;
	endpoint->rcv_urb->actual_length = 0;

	udc_endpoint_read(endpoint);
    }
    else if (endpoint->rcv_urb->actual_length) {

#ifdef __DEBUG_FILE_STORAGE__
	{
	    int i;
	    unsigned int nb = endpoint->rcv_urb->actual_length;
	    unsigned char *src = (unsigned char *) endpoint->rcv_urb->buffer;

	    printf("%d bytes rcvd:", nb);
	    for (i = 0; i < nb; i++) {
		if (!(i % 16)) printf("\n%d: ", i); 
		printf("%02x ", src[i]);
	    }
	    printf("\n");
	}
#endif

	file_storage_parse_cmd(endpoint, endpoint->rcv_urb->buffer);

	endpoint->rcv_urb->actual_length = 0;
    }

    return 0;
}

extern const u8 *get_board_id16(void);

int file_storage_enable(int exit_voltage) 
{
    int ret;
    unsigned short voltage;

    if (udc_init()) {
	puts("Error initializing USB!\n");
	return 1;
    }
    
    // get serial number
    memcpy(serial_number, get_board_id16(), SERIAL_NUM_LEN);
    serial_number[SERIAL_NUM_LEN] = 0;

    file_storage_init_strings ();
    file_storage_init_instances ();
	
    udc_startup_events(device_instance);
    udc_connect();
    
    file_storage_exit = 0;

    while (!file_storage_exit) {
	udc_irq();

#if defined(CONFIG_PMIC)
	/* voltage will be read in the usb driver 
	   here we just sample it */
	ret = pmic_adc_read_last_voltage(&voltage);
	if (ret && (voltage >= exit_voltage)) {
	    break;
	}
#endif

	if (file_storage_configured) {
	    file_storage_check_for_data();
	}

    }

    udc_disable();

    return 0;
}
