/*
 * mx50_yoshi_board.h 
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

#ifndef __MX50_YOSHI_BOARD_H__
#define __MX50_YOSHI_BOARD_H__

#define MEMORY_TYPE_UNKNOWN	0
#define MEMORY_TYPE_MDDR	1
#define MEMORY_TYPE_LPDDR2	2

typedef struct board_type {
    const char *id;  /* Tattoo + rev */
    const char *name;
    unsigned char mem_type;
    unsigned int mem_size;
} board_type;


static const struct board_type boards[] = {
    {
	.id = "00001",
	.name = "Yoshi 1.0",
	.mem_type = MEMORY_TYPE_MDDR,
	.mem_size = (512 * 1024 * 1024),
    },
    {
	.id = "00002",
	.name = "Yoshi 1.5",
	.mem_type = MEMORY_TYPE_LPDDR2,
	.mem_size = (512 * 1024 * 1024),
    },
    {
	.id = "00003",
	.name = "Yoshi 3.0",
	.mem_type = MEMORY_TYPE_LPDDR2,
	.mem_size = (512 * 1024 * 1024),
    },
    {
	.id = "00004",
	.name = "Yoshi 4.0",
	.mem_type = MEMORY_TYPE_MDDR,
	.mem_size = (512 * 1024 * 1024),
    },
    {
	.id = "00005",
	.name = "Yoshi 5.0",
	.mem_type = MEMORY_TYPE_MDDR,
	.mem_size = (512 * 1024 * 1024),
    },
    {
	.id = "003",
	.name = "Tequila",
	.mem_type = MEMORY_TYPE_MDDR,
	.mem_size = (256 * 1024 * 1024),
    },
    {
	.id = "005",
	.name = "Whitney",
	.mem_type = MEMORY_TYPE_MDDR,
	.mem_size = (256 * 1024 * 1024),
    },
    {
	.id = "006",
	.name = "Whitney WFO",
	.mem_type = MEMORY_TYPE_MDDR,
	.mem_size = (256 * 1024 * 1024),
    },
};

#define NUM_KNOWN_BOARDS (sizeof(boards)/sizeof(boards[0]))

#define PARTITION_FILL_SPACE	-1

typedef struct partition_info_t {
    const char *name;
    unsigned int address;
    unsigned int size;
    unsigned int partition;
} partition_info_t;

static const struct partition_info_t partition_info[] = {
    {
	.name = "bootloader",
	.address = 0,
	.size = (376*1024), /* 376 KiB */
	.partition = 1,
    },
    {
	.name = "prod",
	.address = 0x0, /* overlap with bootloader */
	.size = (120*1024), /* 120 KiB */
	.partition = 1,
    },
    {
	.name = "bist",
	.address = CONFIG_MMC_BIST_ADDR, /* overlap with bootloader */
	.size = CONFIG_MMC_BIST_SIZE, /* 256 KiB */
	.partition = 1,
    },
    {
	.name = "userdata",
	.address = CONFIG_MMC_USERDATA_ADDR,
	.size = CONFIG_MMC_USERDATA_SIZE,  /* 5 KiB */
	.partition = 1,
    },
    {
    .name = "userpartition",
    .address = 0,
    .size = PARTITION_FILL_SPACE,  /* based on MMC size */
    .partition = 0,
    },
    {
	.name = "mbr",
	.address = 0,
	.size = 1024,  /* 1 KiB */
	.partition = 0,
    },
    {
	.name = "kernel",
	.address = CONFIG_MMC_BOOTFLASH_ADDR,
	.size = CONFIG_MMC_BOOTFLASH_SIZE,  /* 14 MiB */
	.partition = 0,
    },
    {
	.name = "diags_kernel",
	.address = CONFIG_MMC_BOOTDIAGS_ADDR,
	.size = CONFIG_MMC_BOOTFLASH_SIZE,  /* 14 MiB */
	.partition = 0,
    },
    {
	.name = "system",
	.address = 0x2000000,
	.size = (350*1024*1024),  /* 350 MiB */
	.partition = 0,
    },
    {
	.name = "diags",
	.address = 0x17E00000,
	.size = (64*1024*1024),  /* 64 MiB */
	.partition = 0,
    },
    {
	.name = "data",
	.address = 0x1BE00000,
	.size = PARTITION_FILL_SPACE,  /* based on MMC size */
	.partition = 0,
    }
};

#define CONFIG_NUM_PARTITIONS (sizeof(partition_info)/sizeof(partition_info[0]))

typedef struct nvram_t {
    const char *name;
    unsigned int offset;
    unsigned int size;
} nvram_t;

static const struct nvram_t nvram_info[] = {
    {
	.name = "serial",
	.offset = 0x00,
	.size = 16,
    },
    {
	.name = "accel",
	.offset = 0x20,
	.size = 16,
    },
    {
	.name = "mac",
	.offset = 0x30,
	.size = 12,
    },
    {
	.name = "sec",
	.offset = 0x40,
	.size = 20,
    },
    {
	.name = "pcbsn",
	.offset = 0x60,
	.size = 16,
    },
    {
	.name = "bootmode",
	.offset = 0x1000,
	.size = 16,
    },
    {
	.name = "postmode",
	.offset = 0x1010,
	.size = 16,
    },
};

#define CONFIG_NUM_NV_VARS (sizeof(nvram_info)/sizeof(nvram_info[0]))

#endif /* __MX50_YOSHI_CFG_H__ */
