/*
 * fastboot.c
 *
 * (C) Copyright 2010 Amazon Technologies, Inc.  All rights reserved.
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

#include <common.h>
#include <command.h>
#include <usbdevice.h>
#include <usb_defs.h>
#include <usbdescriptors.h>
#include <mmc.h>
#include <usb/fastboot.h>

#include <u-boot/md5.h>

#define ERR(fmt, args...)                               \
	serial_printf("ERROR : [%s] %s:%d: "fmt,              \
                __FILE__,__FUNCTION__,__LINE__, ##args)

//#define __DEBUG_FASTBOOT__ 1
#ifdef __DEBUG_FASTBOOT__
#define DBG(fmt,args...)                                \
  serial_printf("[%s] %s:%d: "fmt,                      \
                __FILE__,__FUNCTION__,__LINE__, ##args)
#else
#define DBG(fmt,args...)
#endif

// BEN TODO
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

#define FASTBOOT_RX_MAX	16384

#define FASTBOOT_MAXPOWER 0xFA

#define INVALID_PARTITION 0xFFFFFFFF

#define BUFFER_LEN CONFIG_MMC_MAX_TRANSFER_SIZE

#ifdef CONFIG_FASTBOOT_TEMP_BUFFER
static unsigned char *transfer_buffer = (unsigned char *) CONFIG_FASTBOOT_TEMP_BUFFER;
#else
unsigned char transfer_buffer[BUFFER_LEN] __attribute__ ((__aligned__(4096)));
#endif

extern int do_reset (cmd_tbl_t *cmdtp, int flag, int argc, char *argv[]);

#if CONFIG_POST & CONFIG_SYS_POST_MMC_CRC32
extern int mmc_crc32_test (uint part, uint start, int size, uint crc);
#endif

/*
 * Serial number
 */
#define SERIAL_NUM_LEN 16

static char serial_number[SERIAL_NUM_LEN + 1];

/*
 * Instance variables
 */
static int fastboot_post_flags = 0;
static int fastboot_configured = 0;
static int fastboot_addressed = 0;
static int fastboot_exit = 0;
static unsigned int fastboot_mmc_device = 0;
static unsigned int fastboot_mmc_partition = FASTBOOT_USE_DEFAULT;

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

static struct usb_string_descriptor *fastboot_string_table[STR_COUNT];

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
	.idProduct =		cpu_to_le16(CONFIG_USBD_PRODUCTID_FASTBOOT),
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

struct fastboot_config_desc {

  struct usb_configuration_descriptor configuration_desc;
  struct usb_interface_descriptor interface_desc[NUM_INTERFACES];
  struct usb_endpoint_descriptor data_endpoints[NUM_ENDPOINTS];

} __attribute__((packed));

static struct fastboot_config_desc fastboot_hs_configuration_descriptors[NUM_CONFIGS] ={
  {
    .configuration_desc ={
	    .bLength = sizeof(struct usb_configuration_descriptor),
	    .bDescriptorType = USB_DT_CONFIG,
	    .wTotalLength = cpu_to_le16(sizeof(struct fastboot_config_desc)),
	    .bNumInterfaces = NUM_INTERFACES,
	    .bConfigurationValue = 1,
	    .bmAttributes = BMATTRIBUTE_SELF_POWERED|BMATTRIBUTE_RESERVED,
	    .bMaxPower = FASTBOOT_MAXPOWER
    },
    .interface_desc = {
      {
        .bLength  =
        sizeof(struct usb_interface_descriptor),
        .bDescriptorType = USB_DT_INTERFACE,
        .bInterfaceNumber = 0,
        .bAlternateSetting = 0,
        .bNumEndpoints = NUM_ENDPOINTS,
        .bInterfaceClass = 0xFF,
        .bInterfaceSubClass = 0x42,
        .bInterfaceProtocol = 0x03,
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

static struct fastboot_config_desc fastboot_fs_configuration_descriptors[NUM_CONFIGS] ={
  {
    .configuration_desc ={
	    .bLength = sizeof(struct usb_configuration_descriptor),
	    .bDescriptorType = USB_DT_CONFIG,
	    .wTotalLength = cpu_to_le16(sizeof(struct fastboot_config_desc)),
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
        .bInterfaceClass = 0xFF,
        .bInterfaceSubClass = 0x42,
        .bInterfaceProtocol = 0x03,
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

static void fastboot_init_endpoints (struct usb_device_instance *device)
{
	int i;
	struct usb_endpoint_descriptor **epdesc;

	bus_instance->max_endpoints = NUM_ENDPOINTS + 1;

	if (device->speed == USB_SPEED_HIGH) {
    DBG("using HIGH speed descriptors\n");
    epdesc = hs_ep_descriptor_ptrs;
    fastboot_post_flags |= 1;		// POST test requires high speed host
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
  		le16_to_cpu(epdesc[0]->wMaxPacketSize);

    endpoint_instance[i].tx_packetSize =
      le16_to_cpu(epdesc[0]->wMaxPacketSize);

    endpoint_instance[i].tx_attributes =
      epdesc[i - 1]->bmAttributes;

    endpoint_instance[i].tx_attributes =
      epdesc[i - 1]->bmAttributes;
  
    urb_link_init (&endpoint_instance[i].rcv);

    urb_link_init (&endpoint_instance[i].rdy);

    urb_link_init (&endpoint_instance[i].tx);

    urb_link_init (&endpoint_instance[i].done);

    if (endpoint_instance[i].endpoint_address & USB_DIR_IN) {
      endpoint_instance[i].tx_urb =
		    usbd_alloc_urb (device,
                        &endpoint_instance[i]);
    } else {
      endpoint_instance[i].rcv_urb =
		    usbd_alloc_urb (device,
                        &endpoint_instance[i]);
    }

    udc_setup_ep (device, i, &endpoint_instance[i]);
	}

}

static void fastboot_event_handler (struct usb_device_instance *device,
                                    usb_device_event_t event, int data)
{
  switch (event) {
  case DEVICE_RESET:
  case DEVICE_BUS_INACTIVE:
  case DEVICE_DE_CONFIGURED:

    // if the cable is unplugged, exit

    if (fastboot_addressed)
	    fastboot_exit = 1;


    fastboot_addressed = 0;
    fastboot_configured = 0;
    break;

  case DEVICE_ADDRESS_ASSIGNED:
    fastboot_addressed = 1;
    break;

  case DEVICE_CONFIGURED:
    {

	    struct usb_endpoint_instance *endpoint = &endpoint_instance[EP_OUT];
    
	    fastboot_configured = 1;
	    fastboot_init_endpoints (device);
      udc_irq();
	    endpoint->rcv_urb->buffer = (u8 *) endpoint->rcv_urb->buffer_data;
	    endpoint->rcv_urb->buffer_length = FASTBOOT_RX_MAX;  // max fastboot command length
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

static void fastboot_init_strings (void)
{
  struct usb_string_descriptor *string;

  fastboot_string_table[STR_LANG] =
    (struct usb_string_descriptor*)wstrLang;

  string = (struct usb_string_descriptor *) wstrManufacturer;
  string->bLength = sizeof(wstrManufacturer);
  string->bDescriptorType = USB_DT_STRING;
  str2wide (CONFIG_USBD_MANUFACTURER, string->wData);
  fastboot_string_table[STR_MANUFACTURER]=string;


  string = (struct usb_string_descriptor *) wstrProduct;
  string->bLength = sizeof(wstrProduct);
  string->bDescriptorType = USB_DT_STRING;
  str2wide (CONFIG_USBD_PRODUCT_NAME, string->wData);
  fastboot_string_table[STR_PRODUCT]=string;

  string = (struct usb_string_descriptor *) wstrSerial;
  string->bLength = sizeof(wstrSerial);
  string->bDescriptorType = USB_DT_STRING;
  str2wide (serial_number, string->wData);
  fastboot_string_table[STR_SERIAL]=string;

  /* Now, initialize the string table for ep0 handling */
  usb_strings = fastboot_string_table;
}

static void fastboot_init_instances (void)
{
  int i;

  /* initialize device instance */
  memset (device_instance, 0, sizeof (struct usb_device_instance));
  device_instance->device_state = STATE_INIT;
  device_instance->device_descriptor = &device_descriptor;
  device_instance->qualifier_descriptor = &qualifier_descriptor;
  device_instance->event = fastboot_event_handler;
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
    &fastboot_hs_configuration_descriptors;
  fs_configuration_descriptor = (struct usb_configuration_descriptor*)
    &fastboot_fs_configuration_descriptors;

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
  hs_ep_descriptor_ptrs[0] = &fastboot_hs_configuration_descriptors[0].data_endpoints[0];
  hs_ep_descriptor_ptrs[1] = &fastboot_hs_configuration_descriptors[0].data_endpoints[1];
  fs_ep_descriptor_ptrs[0] = &fastboot_fs_configuration_descriptors[0].data_endpoints[0];
  fs_ep_descriptor_ptrs[1] = &fastboot_fs_configuration_descriptors[0].data_endpoints[1];

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

static int fastboot_send_reply_actual(char *reply, unsigned int replylen) {
    
  struct usb_endpoint_instance *endpoint = &endpoint_instance[EP_IN];
  struct urb *current_urb = NULL;

  current_urb = endpoint->tx_urb;

  DBG("begin.  reply=%s (%d)\n", reply, replylen);

  if (replylen) {

    int space_avail;
    int popnum;
    int total = 0;

    /* Break buffer into urb sized pieces,
     * and link each to the endpoint
     */
    while (replylen > 0) {

      if (!current_urb) {
        printf ("%s: current_urb is NULL, length %d\n", __FUNCTION__, replylen);
        return total;
      }

      current_urb->actual_length = 0;
      current_urb->buffer = (unsigned char *) current_urb->buffer_data;
      current_urb->buffer_length = (URB_BUF_SIZE * sizeof(current_urb->buffer_data[0]));
      current_urb->status = 0;

      space_avail = current_urb->buffer_length;
      popnum = MIN (space_avail, replylen);
      if (popnum == 0)
        break;

      memcpy(current_urb->buffer, reply + total, popnum);

      current_urb->actual_length += popnum;
      total += popnum;
      replylen -= popnum;

      if(udc_endpoint_write (endpoint)){
        /* Write pre-empted by RX */
        ERR("ep write fail!\n");
        return -1;
      }

      // wait for complete irq
      while (current_urb->actual_length)
        udc_irq();
    
    }/* end while */
  
  
    return total;
  }
    
  return 0;
}

static int fastboot_send_reply(char *reply) {
  return fastboot_send_reply_actual(reply, strlen(reply));
}

static unsigned hex2unsigned(const char *x)
{
  unsigned n = 0;

  while(*x) {
    switch(*x) {
    case '0': case '1': case '2': case '3': case '4':
    case '5': case '6': case '7': case '8': case '9':
      n = (n << 4) | (*x - '0');
      break;
    case 'a': case 'b': case 'c':
    case 'd': case 'e': case 'f':
      n = (n << 4) | (*x - 'a' + 10);
      break;
    case 'A': case 'B': case 'C':
    case 'D': case 'E': case 'F':
      n = (n << 4) | (*x - 'A' + 10);
      break;
    default:
      return n;
    }
    x++;
  }

  return n;
}

static void num_to_hex(int size, unsigned n, char *out)
{
  static char tohex[16] = "0123456789abcdef";
  int i;
  for(i = size - 1; i >= 0; i--) {
    out[i] = tohex[n & 15];
    n >>= 4;
  }
  out[size] = 0;
}

static unsigned int fastboot_find_partition(const char *partition_name, 
                                            unsigned int *partition_size,
                                            unsigned int *partition_index,
                                            unsigned int *mmc_partition_number)
{
  int i;
  unsigned int offset = 0;
  int len;

  *partition_size = 0;
  *partition_index = 0;
  *mmc_partition_number = 0;

  for (i = 0; i < CONFIG_NUM_PARTITIONS; i++) {

    len = strlen(partition_info[i].name);
    if (strncmp(partition_name, partition_info[i].name, len) == 0) {

	    offset = 0;

	    *partition_index = i;

	    // check for offset
	    if (partition_name[len] == ':') {
        offset = hex2unsigned(partition_name + (len + 1));
        DBG("offset: %d\n", offset);
	    }
	
	    if (partition_info[i].size == PARTITION_FILL_SPACE) {
        *partition_size = PARTITION_FILL_SPACE;
	    } else {
        *partition_size = (partition_info[i].size - offset);
	    }

#ifdef CONFIG_BOOT_PARTITION_ACCESS
	    *mmc_partition_number = partition_info[i].partition;
#endif
	    return (partition_info[i].address + offset);
    }
  } 

  return INVALID_PARTITION;
}

extern const char version_string[];
extern const u8 *get_board_id16(void);

extern void board_reset(void);
extern void board_power_off(void);
extern unsigned int get_dram_size(void);

extern int do_pass (cmd_tbl_t *cmdtp, int flag, int argc, char *argv[]);
extern int do_fail (cmd_tbl_t *cmdtp, int flag, int argc, char *argv[]);

static int kernel_addr = CONFIG_LOADADDR;
static unsigned int kernel_size = 0;

// TODO not actual size since we're now using CONFIG_LOADADDR
// rather than statically allocated memory
#define REPLY_BUF_LEN	4096 

void requeue_command_buffer(struct usb_endpoint_instance *endpoint) {

  endpoint->rcv_urb->buffer = (u8 *) endpoint->rcv_urb->buffer_data;
  endpoint->rcv_urb->buffer_length = FASTBOOT_RX_MAX;  
  endpoint->rcv_urb->status = 0;
  endpoint->rcv_urb->actual_length = 0;

  udc_endpoint_read(endpoint);
}

// TODO split into init, update and finalize
/*
  uint32_t adler32(const void *buf, size_t buflength) {
  const uint8_t* buffer = (const uint8_t*) buf;

  uint32_t s1 = 1;
  uint32_t s2 = 0;
  size_t n;

  for(n = 0; n < buflength; n++) {
  s1 = (s1 + buffer[n]) % 65521;
  s2 = (s2 + s1) % 65521;
  }     
  return (s2 << 16) | s1;
  }
*/
/*
 * Parse a fastboot cmd.  cmdbuf must be NULL-terminated
 */
static void fastboot_parse_cmd(char *cmdbuf) 
{
  char* reply_buf = NULL;
  unsigned int rx_length;
  unsigned long int flash_addr;
  unsigned int upload_size;
  unsigned int upload_chunk_size;
  unsigned int uploaded;
  unsigned int ram_free;
  unsigned char* src;
  unsigned int pktsize, len, dest;
  unsigned int part_size;
  unsigned int part_index;
  unsigned int mmc_partition;
  unsigned int mmc_source;
  unsigned char* ram_dest;
  unsigned int count;
  uint32_t crc_cac = ~0U;
  int i;
  int ret;
#ifdef CONFIG_GENERIC_MMC
  struct mmc *mmc;
#endif

  reply_buf = (unsigned char *) kernel_addr;

  struct usb_endpoint_instance *endpoint = &endpoint_instance[EP_OUT];

  if (strncmp(cmdbuf, "getvar", 6) == 0) {
    /* Read a config/version variable from the bootloader. 
       The variable contents will be returned after the
       OKAY response. */

    // get the variable name
    cmdbuf += 7;

    if (strncmp(cmdbuf, "version-bootloader", 18) == 0) {
	    /* Version string for the Bootloader. */
	    sprintf(reply_buf, "OKAY%s", version_string);
	    fastboot_send_reply(reply_buf);
    }
    else if (strncmp(cmdbuf, "version", 7) == 0) {
	    /* Version of FastBoot protocol supported. */
	    fastboot_send_reply("OKAY0.3");
    }
    else if (strncmp(cmdbuf, "product", 7) == 0) {
	    /* Name of the product */
	    fastboot_send_reply("OKAYKindle");
    }
    else if (strncmp(cmdbuf, "serialno", 8) == 0) {
      /* Product serial number */
	    sprintf(reply_buf, "OKAY%s", serial_number);
	    fastboot_send_reply(reply_buf);
    }
    else {
	    fastboot_send_reply("OKAYno");
    }
  } else if (strncmp(cmdbuf, "download", 8) == 0) {
    /* Write data to memory which will be later used
       by "boot", "ramdisk", "flash", etc.  The client
       will reply with "DATA%08x" if it has enough 
       space in RAM or "FAIL" if not.  The size of
       the download is remembered. */
      
    unsigned int bytes_rcvd;
    unsigned int rx_addr;

    rx_length = hex2unsigned(cmdbuf + 9);

    /* tell the host our max size if the download is too large */
    if (rx_length > CONFIG_FASTBOOT_MAX_DOWNLOAD_LEN) {
      printf("capping download at 0x%x\n", CONFIG_FASTBOOT_MAX_DOWNLOAD_LEN);
      rx_length = CONFIG_FASTBOOT_MAX_DOWNLOAD_LEN;
    }
    kernel_size = rx_length;
    rx_addr = kernel_addr;

    DBG("recv data addr=%x size=%x\n", rx_addr, rx_length); 
        
    strcpy(reply_buf,"DATA");
    num_to_hex(8, rx_length, reply_buf + 4);
    reply_buf[12] = 0;

    fastboot_send_reply(reply_buf);

    bytes_rcvd = 0;

    printf("downloading\n");
    while (rx_length > bytes_rcvd) {
      // queue data buffer
      endpoint->rcv_urb->buffer = (u8 *) rx_addr;
      endpoint->rcv_urb->buffer_length = MIN((rx_length - bytes_rcvd), FASTBOOT_RX_MAX);
      endpoint->rcv_urb->status = 0;
      endpoint->rcv_urb->actual_length = 0;
      udc_endpoint_read(endpoint);

      while (!endpoint->rcv_urb->actual_length && !endpoint->rcv_urb->status) {
        udc_irq();
      }

      bytes_rcvd += endpoint->rcv_urb->actual_length;
      rx_addr += endpoint->rcv_urb->actual_length;

      //DBG(" rcvd: %d bytes.  %d of %d\n", endpoint->rcv_urb->actual_length, bytes_rcvd, rx_length);

      if (!(bytes_rcvd % 0x2000000))
        printf(".");
	    
      if (endpoint->rcv_urb->status) {
        ERR("DATA receive failure: %d\n", endpoint->rcv_urb->status);
        fastboot_send_reply("FAILdownload failure");
        goto out;
      }
    }

    printf("done\n");
      
    fastboot_send_reply("OKAY");

    // upload mmc contents to fastboot client
  } else if (strncmp(cmdbuf, "upload", 6) == 0) {

    printf("Upload command received\n");
    
    mmc_source = 0; // offset into mmc flash memory

    // assuming that mmc device has already been initialized
    // which should be the case if fastboot_mmc_device is the boot device
    // which it always is on kindle 4th and 5th gen

    mmc = find_mmc_device(fastboot_mmc_device);
    if(mmc == NULL) {
      fastboot_send_reply("FAILCouldn't find flash device");
      goto out;
    }
    
    // calculate how much free SDRAM we have
    // CONFIG_SYS_SDRAM_BASE is where the SDRAM is memory-mapped (0x70000000)
    // CONFIG_LOADADDR is where the kernel is loaded into SDRAM (0x70800000)
    ram_free = CONFIG_SYS_SDRAM_SIZE - (CONFIG_LOADADDR - CONFIG_SYS_SDRAM_BASE) - 1024 * 1024; // TODO skipping the last megabyte for now


    ram_dest = (unsigned char *) CONFIG_LOADADDR; // where in ram to copy

    strcpy(ram_dest, "ATAD");
    num_to_hex(32, mmc->capacity, ram_dest + 4); // add the data length (with a null terminator)

    fastboot_send_reply_actual(ram_dest, 4 + 33);
      
    upload_size = mmc->capacity;

    printf("Starting upload of %llu bytes\n", mmc->capacity);
    
    while(upload_size > 0) {
      
      uploaded = 0;

      ram_dest = (unsigned char *) CONFIG_LOADADDR; // where in ram to copy

      // this is all we can fit into ram
      upload_chunk_size = MIN(upload_size, ram_free);

      printf("Reading %u byte chunk from flash memory into ram\n", upload_chunk_size);
      
      while(upload_chunk_size > 0) {

        pktsize = MIN(upload_chunk_size, CONFIG_MMC_MAX_TRANSFER_SIZE);

        if(mmc_read(fastboot_mmc_device, mmc_source, ram_dest, pktsize)) {
          fastboot_send_reply("FAILFlash read fail!");
          goto out;
        }

        printf("Computing CRC32 of previous chunk\n");
        crc_cac = crc32(crc_cac, (unsigned char*) ram_dest, pktsize);
        mmc_source += pktsize;
        ram_dest += pktsize;
        upload_chunk_size -= pktsize;
        uploaded += pktsize;
      }

      printf("Sending %u bytes to client\n", uploaded);
      fastboot_send_reply_actual((unsigned char *) CONFIG_LOADADDR, uploaded);

      upload_size -= uploaded;
    }
    
    crc_cac=~crc_cac; // invert it at the end
    //      num_to_hex(32, crc_cac, (unsigned char *) CONFIG_LOADADDR); // add the data length (with a null terminator)
      
    // send crc32 (4 bytes)
    printf("CRC32: %x\n", crc_cac);
    fastboot_send_reply_actual((unsigned char *) &crc_cac, 4);

    goto out;

  } else if (strncmp(cmdbuf, "flash", 5) == 0) {

    flash_addr = strtoul(cmdbuf + 6, NULL, 16);

#ifdef CONFIG_GENERIC_MMC
    mmc = find_mmc_device(fastboot_mmc_device);
    if (mmc == NULL) {
	    fastboot_send_reply("FAILCouldn't find flash device");
	    goto out;
    }

    // Make sure that the mmc device has been inited
    if (fastboot_mmc_device != CONFIG_MMC_BOOTFLASH) {
	    if (mmc_init(mmc)) {
        fastboot_send_reply("FAILCouldn't init flash device");
        goto out;			
	    }
    }

    /* Calculate the actual partition size based on MMC flash size */
    if (part_size == PARTITION_FILL_SPACE) {
	    u64 real_part_size = mmc->capacity - flash_addr;

	    if (kernel_size > real_part_size) {
        fastboot_send_reply("FAILFile too large for partition");
        goto out;
	    }
	    
    } else 
#endif
      /* make sure we have enough space for the file */
      if (kernel_size > part_size) {
        fastboot_send_reply("FAILFile too large for partition");
        goto out;
      }

    src = (unsigned char *) kernel_addr;
    len = kernel_size;
    dest = flash_addr;

#ifdef CONFIG_BOOT_PARTITION_ACCESS

    /* Use override partition if specified */
    if (fastboot_mmc_partition != FASTBOOT_USE_DEFAULT) {
	    mmc_partition = fastboot_mmc_partition;
    }

    if (mmc_partition > 0) {
	    if (mmc_switch_partition(mmc, mmc_partition, 1) < 0) {
        fastboot_send_reply("FAILCouldn't init flash partition");
        goto out;			
	    }
    }
#endif

    DBG("flashing %s to MMC%d, partition %d\n", cmdbuf + 6, fastboot_mmc_device, mmc_partition);

    while (len > 0) {
	    pktsize = MIN(len, BUFFER_LEN);

	    DBG("flash %d bytes to 0x%x from 0x%x\n", pktsize, dest, (unsigned int) src);

	    if (mmc_write(fastboot_mmc_device, src, dest, pktsize)) {
        fastboot_send_reply("FAILFlash write fail!");
        DBG("flash write fail!\n");
        goto out;
	    }

	    src += pktsize;
	    dest += pktsize;
	    len -= pktsize;

	    if (pktsize == BUFFER_LEN) {
        DBG(".");
      }
    }
    DBG("done.\n");

#ifdef CONFIG_BOOT_PARTITION_ACCESS
    /* Switch partition back to user */
    if (mmc_partition > 0) {
	    if (mmc_switch_partition(mmc, 0, 0) < 0) {
        printf("Couldn't switch back to user partition!\n");
	    }
    }
#endif
    fastboot_send_reply("OKAY");
  }
  else if (strncmp(cmdbuf, "eraseall", 8) == 0) {

#ifdef CONFIG_GENERIC_MMC

    mmc = find_mmc_device(fastboot_mmc_device);
    if (mmc == NULL) {
	    fastboot_send_reply("FAILCouldn't find flash device");
	    goto out;
    }

    // Make sure that the mmc device has been inited
    if (fastboot_mmc_device != CONFIG_MMC_BOOTFLASH) {
	    if (mmc_init(mmc)) {
        fastboot_send_reply("FAILCouldn't init flash device");
        goto out;			
	    }
    }
#else
    printf("FAILunsupported!\n");
    goto out;
#endif

    printf("Erasing user partition..\n");
    mmc_erase(mmc, 0, mmc->capacity - 512);
	
#if defined(CONFIG_BOOT_PARTITION_ACCESS) && defined(CONFIG_BOOT_FROM_PARTITION)

    /* Now erase boot partition */
    if (mmc_switch_partition(mmc, CONFIG_BOOT_FROM_PARTITION, 0) < 0) {
	    fastboot_send_reply("FAILCouldn't find flash device");
	    goto out;			
    }
	
    printf("Erasing boot partition..\n");
    mmc_erase(mmc, 0, 0);

    printf("Done\n");

    /* Switch partition back to user */
    if (mmc_switch_partition(mmc, 0, 0) < 0) {
	    printf("Couldn't switch back to user partition!\n");
    }
#endif

    fastboot_send_reply("OKAY");
    
  } else if (strncmp(cmdbuf, "boot", 4) == 0) {
    /*
      TODO remove
    */
    struct MD5Context foo;
    unsigned char bar;
    unsigned char baz[16];
    MD5Init(&foo);
    MD5Update(&foo, &bar, 1);
    MD5Final(baz, &foo);
      

    /* The previously downloaded data is a boot.img
       and should be booted according to the normal
       procedure for a boot.img */
    fastboot_send_reply("OKAY");

    // boot from default location
    if (run_command (getenv ("bootcmd"), 0) < 0)
	    ERR("Couldn't boot with cmd '%s'", getenv("bootcmd"));

  }
  else if (strncmp(cmdbuf, "continue", 8) == 0) {
    /* Kick out to the bootloader prompt */
    fastboot_send_reply("OKAY");

    fastboot_exit = 1;
    return;
  }
  else if (strncmp(cmdbuf, "reboot", 6) == 0) {
    /* Reboot the device. */
    fastboot_send_reply("OKAY");

    do_reset(NULL, 0, 0, NULL);
    return;
  }
#ifdef CONFIG_CMD_HALT
  else if (strncmp(cmdbuf, "powerdown", 9) == 0) {
    /* Power off the device. */
    fastboot_send_reply("OKAY");

    board_power_off();
    return;
  }
#endif
#ifdef CONFIG_CMD_FAIL
  else if (strncmp(cmdbuf, "pass", 4) == 0) {
    /* Flash pass pattern on LED */
    fastboot_send_reply("OKAY");

    do_pass(NULL, 0, 0, NULL);
    return;
  }
  else if (strncmp(cmdbuf, "fail", 4) == 0) {
    /* Flash fail pattern on LED */
    fastboot_send_reply("OKAY");

    do_fail(NULL, 0, 0, NULL);
    return;
  }
#endif
  else {
    fastboot_send_reply("FAILunknown command");
  }
  fastboot_post_flags |= 2;		// POST test requires a command

 out:
  requeue_command_buffer(endpoint);

}

static int fastboot_check_for_data(void) {

  struct usb_endpoint_instance *endpoint =
    &endpoint_instance[1];

  if (endpoint->rcv_urb->status) {
    printf("ERROR\n");
    ERR("error receving usb packet: %d\n", endpoint->rcv_urb->status);

    // requeue command buffer
    endpoint->rcv_urb->buffer = (u8 *) endpoint->rcv_urb->buffer_data;
    endpoint->rcv_urb->buffer_length = FASTBOOT_RX_MAX;  
    endpoint->rcv_urb->status = 0;
    endpoint->rcv_urb->actual_length = 0;

    udc_endpoint_read(endpoint);
  }
  else if (endpoint->rcv_urb->actual_length) {

    unsigned int nb = 0;
    char *src = (char *) endpoint->rcv_urb->buffer;

    /* make sure to null terminate cmd string */
    nb = endpoint->rcv_urb->actual_length;
    src[nb] = 0;

#ifdef __DEBUG_FASTBOOT__
    {
	    int i;
	    printf("bytes rcvd:");
	    for (i = 0; i < nb; i++) {
        if (!(i % 16)) printf("\n%d: ", i); 
        printf("%c ", src[i]);
	    }
	    printf("\n");
    }
#endif

    fastboot_parse_cmd(src);

    endpoint->rcv_urb->actual_length = 0;

    return nb;
  }
  return 0;
}

int fastboot_enable(int dev, int part)
{
  while(1) {

    fastboot_addressed = 0;
    fastboot_configured = 0;
    fastboot_mmc_device = dev;
    fastboot_mmc_partition = part;

    printf("Entering fastboot mode...\n");

    if (fastboot_mmc_partition != FASTBOOT_USE_DEFAULT && fastboot_mmc_partition > 2) {
      printf("Error: partition %d invalid\n", fastboot_mmc_partition);
      return 1;
    }

    if (udc_init()) {
      puts("Error initializing USB!\n");
      return 1;
    }

    // get serial number
    memcpy(serial_number, get_board_id16(), SERIAL_NUM_LEN);
    serial_number[SERIAL_NUM_LEN] = 0;

    fastboot_init_strings ();
    fastboot_init_instances ();
	
    udc_startup_events(device_instance);
    udc_connect();
    
    fastboot_exit = 0;

    while (!fastboot_exit) {
      udc_irq();
    
      if (fastboot_configured) {
        //        printf("FASTBOOT checking\n");
        fastboot_check_for_data();
      }
    }
  }

  udc_disable();

  return 0;
}



