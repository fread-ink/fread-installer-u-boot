/*
 * Copyright 2008-2009 Freescale Semiconductor, Inc.
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

#ifndef SDHC_H
#define SDHC_H

#include <linux/types.h>

/*
 * Controller registers
 */

#define ESDHC_DMA_ADDRESS	0x00

#define ESDHC_BLOCK_SZ_CNT	0x04
#define  ESDHC_BLOCK_SHIFT		(16)

#define ESDHC_ARGUMENT		0x08

#define ESDHC_TRANSFER_MODE	0x0C
#define  ESDHC_TRNS_DMA_ENABLE			((u32)0x00000001)
#define  ESDHC_BLOCK_COUNT_ENABLE_SHIFT         (1)
#define  ESDHC_AC12_ENABLE_SHIFT	        (2)
#define  ESDHC_DATA_TRANSFER_SHIFT		(4)
#define  ESDHC_MULTI_SINGLE_BLOCK_SELECT_SHIFT  (5)
#define  ESDHC_RESPONSE_FORMAT_SHIFT		(16)
#define  ESDHC_CRC_CHECK_SHIFT                  (19)
#define  ESDHC_CMD_INDEX_CHECK_SHIFT            (20)
#define  ESDHC_DATA_PRESENT_SHIFT               (21)
#define  ESDHC_CMD_INDEX_SHIFT                  (24)

#define ESDHC_RESPONSE		0x10

#define ESDHC_BUFFER		0x20

#define ESDHC_PRESENT_STATE	0x24
#define  ESDHC_PRESENT_STATE_CIHB	((u32)0x00000001)
#define  ESDHC_PRESENT_STATE_CDIHB	((u32)0x00000002)

#define ESDHC_HOST_CONTROL 	0x28
#define  ESDHC_CTRL_BUS_WIDTH_MASK	((u32)0x00000006)
#define  ESDHC_CTRL_1BITBUS		((u32)0x00000000)
#define  ESDHC_CTRL_4BITBUS		((u32)0x00000002)
#define  ESDHC_CTRL_8BITBUS		((u32)0x00000004)
#define  ESDHC_ENDIAN_MODE_MASK		((u32)0x00000030)
#define  ESDHC_LITTLE_ENDIAN_MODE	((u32)0x00000020)
#define  ESDHC_HW_BIG_ENDIAN_MODE	((u32)0x00000010)
#define  ESDHC_BIG_ENDIAN_MODE		((u32)0x00000000)
#define  ESDHC_CTRL_DMA_SEL_MASK	((u32)0x00000300)

#define ESDHC_SYSTEM_CONTROL	0x2C
#define  ESDHC_SYSCTL_INITA		((u32)0x08000000)
#define  ESDHC_SOFTWARE_RESET_DATA	((u32)0x04000000)
#define  ESDHC_SOFTWARE_RESET_CMD	((u32)0x02000000)
#define  ESDHC_SOFTWARE_RESET		((u32)0x01000000)
#define  ESDHC_SYSCTL_DATA_TIMEOUT_MASK	((u32)0x000F0000)
#define  ESDHC_SYSCTL_FREQ_MASK		((u32)0x0000FFF0)

#define ESDHC_INT_STATUS	0x30
#define ESDHC_INT_ENABLE	0x34
#define ESDHC_SIGNAL_ENABLE	0x38
#define  ESDHC_INT_RESPONSE		0x00000001
#define  ESDHC_INT_DATA_END		0x00000002
#define  ESDHC_INT_DMA_END		0x00000008
#define  ESDHC_INT_SPACE_AVAIL		0x00000010
#define  ESDHC_INT_DATA_AVAIL		0x00000020
#define  ESDHC_INT_CARD_INSERT		0x00000040
#define  ESDHC_INT_CARD_REMOVE		0x00000080
#define  ESDHC_INT_CARD_INT		0x00000100
#define  ESDHC_INT_ERROR		0x00008000
#define  ESDHC_INT_TIMEOUT		0x00010000
#define  ESDHC_INT_CRC			0x00020000
#define  ESDHC_INT_END_BIT		0x00040000
#define  ESDHC_INT_INDEX		0x00080000
#define  ESDHC_INT_DATA_TIMEOUT		0x00100000
#define  ESDHC_INT_DATA_CRC		0x00200000
#define  ESDHC_INT_DATA_END_BIT		0x00400000
#define  ESDHC_INT_BUS_POWER		0x00800000
#define  ESDHC_INT_ACMD12ERR		0x01000000
#define  ESDHC_INT_DMA_ERROR		0x10000000

#define ESDHC_ACMD12_ERR	0x3C

#define ESDHC_CAPABILITIES	0x40

#define ESDHC_WML_LEV		0x44
#define  ESDHC_READ_WATER_MARK_LEVEL_BL_4       ((u32)0x00000001)
#define  ESDHC_READ_WATER_MARK_LEVEL_BL_8       ((u32)0x00000002)
#define  ESDHC_READ_WATER_MARK_LEVEL_BL_16      ((u32)0x00000004)
#define  ESDHC_READ_WATER_MARK_LEVEL_BL_64      ((u32)0x00000010)
#define  ESDHC_READ_WATER_MARK_LEVEL_BL_512     ((u32)0x00000080)

#define  ESDHC_WRITE_WATER_MARK_LEVEL_BL_4      ((u32)0x00010000)
#define  ESDHC_WRITE_WATER_MARK_LEVEL_BL_8      ((u32)0x00020000)
#define  ESDHC_WRITE_WATER_MARK_LEVEL_BL_16     ((u32)0x00040000)
#define  ESDHC_WRITE_WATER_MARK_LEVEL_BL_64     ((u32)0x00100000)
#define  ESDHC_WRITE_WATER_MARK_LEVEL_BL_512    ((u32)0x00800000)

#define ESDHC_HOST_VERSION		0xFC

/*
 * Board specific defines
 */
#define BLK_LEN				(512)
#define ESDHC_CARD_INIT_TIMEOUT		(64)
#define ESDHC_FIFO_SIZE			(128)

#define ESDHC_MAX_BLOCK_COUNT		((u32)0x0000ffff)
#define ESDHC_MAX_BLOCK_LEN		((u32)0x00001fff)

#define ESDHC_SYSCTL_DATA_TIMEOUT_MAX	((u32)0x000E0000)
#define ESDHC_SYSCTL_IDENT_FREQ_TO2	((u32)0x00002040)
#define ESDHC_SYSCTL_OPERT_FREQ_TO2	((u32)0x00000050)
#define ESDHC_SYSCTL_HS_FREQ_TO2	((u32)0x00000020)

#define  ESDHC_INT_CMD_MASK	(ESDHC_INT_RESPONSE | ESDHC_INT_TIMEOUT | \
		ESDHC_INT_CRC | ESDHC_INT_END_BIT | ESDHC_INT_INDEX)
#define  ESDHC_INT_DATA_MASK	(ESDHC_INT_DATA_END | \
		ESDHC_INT_DATA_AVAIL | ESDHC_INT_SPACE_AVAIL | \
		ESDHC_INT_DATA_TIMEOUT | ESDHC_INT_DATA_CRC | \
		ESDHC_INT_DATA_END_BIT)
#define  ESDHC_INT_DMA_MASK	(ESDHC_INT_DATA_END | \
		ESDHC_INT_DMA_ERROR | ESDHC_INT_ACMD12ERR | \
		ESDHC_INT_DATA_TIMEOUT | ESDHC_INT_DATA_CRC | \
		ESDHC_INT_DATA_END_BIT)
#define  ESDHC_INT_DATA_RE_MASK	(ESDHC_INT_DMA_END | \
		ESDHC_INT_DATA_AVAIL | ESDHC_INT_SPACE_AVAIL)

#define WRITE_READ_WATER_MARK_LEVEL	(ESDHC_WRITE_WATER_MARK_LEVEL_BL_512 | \
					 ESDHC_READ_WATER_MARK_LEVEL_BL_512) 

typedef enum {
	ESDHC1,
	ESDHC2,
	ESDHC3
} esdhc_num_t;

typedef enum {
	WRITE,
	READ,
} xfer_type_t;

typedef enum {
	RESPONSE_NONE,
	RESPONSE_136,
	RESPONSE_48,
	RESPONSE_48_CHECK_BUSY
} response_format_t;


typedef enum {
	DATA_PRESENT_NONE,
	DATA_PRESENT,
} data_present_select;

typedef enum {
	DISABLE,
	ENABLE
} crc_check_enable, cmdindex_check_enable, block_count_enable, acmd12_enable;

typedef enum {
	SINGLE,
	MULTIPLE
} multi_single_block_select;

typedef struct {
	u32 command;
	u32 arg;
	xfer_type_t data_transfer;
	response_format_t response_format;
	data_present_select data_present;
	crc_check_enable crc_check;
	cmdindex_check_enable cmdindex_check;
	block_count_enable block_count_enable_check;
	multi_single_block_select multi_single_block;
    acmd12_enable acmd12_enable_check;
        u32 dma_address;
} esdhc_cmd_t;

typedef struct {
	response_format_t format;
	u32 cmd_rsp0;
	u32 cmd_rsp1;
	u32 cmd_rsp2;
	u32 cmd_rsp3;
} esdhc_resp_t;

typedef enum {
	BIG_ENDIAN,
	HALF_WORD_BIG_ENDIAN,
	LITTLE_ENDIAN
} endian_mode_t;

typedef enum {
    HIGHSPEED_FREQ = 52000,   /* in kHz */
    OPERATING_FREQ = 20000,   /* in kHz */
    IDENTIFICATION_FREQ = 400   /* in kHz */
} sdhc_freq_t;

enum esdhc_data_status {
	ESDHC_DATA_ERR = 3,
	ESDHC_DATA_OK = 4
};

enum esdhc_int_cntr_val {
	ESDHC_INT_CNTR_END_CD_RESP = 0x4,
	ESDHC_INT_CNTR_BUF_WR_RDY = 0x8
};

enum esdhc_reset_status {
	ESDHC_WRONG_RESET = 0,
	ESDHC_CORRECT_RESET = 1
};

typedef enum {
	WEAK = 0,
	STRONG = 1
} esdhc_pullup_t;

extern u32 interface_reset(void);
extern void interface_configure_clock(sdhc_freq_t);
extern void interface_read_response(esdhc_resp_t *);
extern u32 interface_send_cmd_wait_resp(esdhc_cmd_t *);
#ifndef CONFIG_FSL_ESDHC_DMA
extern u32 interface_data_read(u32 *, u32);
extern u32 interface_data_write(u32 *, u32);
#endif
extern u32 interface_set_bus_width(u32 bus_width);
extern void interface_config_block_info(u32 blk_len, u32 nob);
/*================================================================================================*/
#endif  /* ESDHC_H */
