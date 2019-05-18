/*
 * cmd_pmic.c 
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
#include <command.h>
#include <mmc.h>
#include <idme.h>

#define MMC_BLOCK_SIZE	512

#ifdef CONFIG_MX35
#define BOARDID_FLASH_ADDR	0xA00
#define BOARDID_FLASH_OFFSET	0x1F0
#define BOARDID_FLASH_LEN	16
#endif

static unsigned char varbuf[MMC_BLOCK_SIZE];

extern int setup_board_info(void);

static const struct nvram_t *idme_find_var(const char *name) 
{
    int i, len;

    for (i = 0; i < CONFIG_NUM_NV_VARS; i++) {

	len = strlen(nvram_info[i].name);
	if (strncmp(name, nvram_info[i].name, len) == 0) {
	    return &nvram_info[i];
	}
    } 

    return NULL;
}

int idme_get_var(const char *name, char *buf, int buflen)
{
    int ret;
    int address, block, offset, size;
#if defined(CONFIG_BOOT_PARTITION_ACCESS) && defined(CONFIG_BOOT_FROM_PARTITION)
    struct mmc *mmc;
#endif

    buf[0] = 0;

#ifdef CONFIG_MX35
    if (strcmp(name, "boardid") == 0) {

	/* BEN - boardid is stored separately on mx35 */
	address = BOARDID_FLASH_ADDR;
	offset = BOARDID_FLASH_OFFSET;
	size = BOARDID_FLASH_LEN;
    }
    else 
#endif
    {
	const struct nvram_t *nv;

	nv = idme_find_var(name);
	if (!nv) {
	    printf("Error! Couldn't find NV variable %s\n", name);
	    return -1;
	}

	block = nv->offset / MMC_BLOCK_SIZE;
	address = CONFIG_MMC_USERDATA_ADDR + (block * MMC_BLOCK_SIZE);
	offset = nv->offset % MMC_BLOCK_SIZE;
	size = nv->size;
    }

#if defined(CONFIG_BOOT_PARTITION_ACCESS) && defined(CONFIG_BOOT_FROM_PARTITION)

    mmc = find_mmc_device(CONFIG_MMC_BOOTFLASH);
    if (mmc == NULL) {
	printf("Error: Couldn't find flash device");
	return -1;
    }

    if (mmc_switch_partition(mmc, CONFIG_BOOT_FROM_PARTITION, 0) < 0) {
	printf("%s ERROR: couldn't switch to boot partition\n", __FUNCTION__);
	return -1;
    }
#endif

    ret = mmc_read(CONFIG_MMC_BOOTFLASH, address, varbuf, MMC_BLOCK_SIZE);
    if (ret) {
	printf("%s error! Couldn't read vars from partition\n", __FUNCTION__);
	/* need to switch back to user partition even on error */
    }

#if defined(CONFIG_BOOT_PARTITION_ACCESS) && defined(CONFIG_BOOT_FROM_PARTITION)
    if (mmc_switch_partition(mmc, 0, 0) < 0) {
	printf("%s ERROR: couldn't switch back to user partition\n", __FUNCTION__);
	return -1;
    }
#endif
    
    if (ret) {
	return ret;
    }

    // buffer is too small, return needed size
    if (size + 1 > buflen) {
	return (size + 1);
    }

    memcpy(buf, varbuf + offset, size);
    buf[size] = 0;

    return 0;
}


