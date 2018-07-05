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

static int idme_print_vars(void) 
{
    int i;
    char value[65];

    for (i = 0; i < CONFIG_NUM_NV_VARS; i++) {

	idme_get_var(nvram_info[i].name, value, sizeof(value));
	value[nvram_info[i].size] = 0;

	printf("%s: %s\n", nvram_info[i].name, value);
    }

    return 0;
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

int idme_update_var(const char *name, const char *value) 
{
    int ret = 0, sz;
    int address, block, offset, size;
#if defined(CONFIG_BOOT_PARTITION_ACCESS) && defined(CONFIG_BOOT_FROM_PARTITION)
    struct mmc *mmc;
#endif

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
	printf("Error! Couldn't read NV variables. (%d)\n", ret);
	goto out;
    }

    // clear out old value
    memset(varbuf + offset, 0, size);

    if (value != NULL) {
	// copy in new value
	sz = MIN(size, strlen(value));
	memcpy(varbuf + offset, value, sz);
    }

    ret = mmc_write(CONFIG_MMC_BOOTFLASH, varbuf, address, MMC_BLOCK_SIZE);
    if (ret) {
	printf("Error! Couldn't write NV variables. (%d)\n", ret);
	/* need to switch back to user partition even on error */
    }

  out:
#if defined(CONFIG_BOOT_PARTITION_ACCESS) && defined(CONFIG_BOOT_FROM_PARTITION)
    if (mmc_switch_partition(mmc, 0, 0) < 0 || ret) {
	printf("%s ERROR: couldn't switch back to user partition\n", __FUNCTION__);
	return -1;
    }
#endif

    /* Make sure that the board info globals (DSN/pcbsn) are updated as well */
    setup_board_info();

    return ret;
}

int idme_check_update(void) {

    int ret = 0;

#ifdef CONFIG_IDME_UPDATE
    struct mmc *mmc;
    int len = 0;

    mmc = find_mmc_device(CONFIG_MMC_BOOTFLASH);
    if (mmc == NULL) {
	printf("%s Error: Couldn't find flash device", __FUNCTION__);
	return 0;
    }

    ret = mmc_read(CONFIG_MMC_BOOTFLASH, CONFIG_IDME_UPDATE_ADDR, varbuf, MMC_BLOCK_SIZE);
    if (ret) {
	printf("Error! Couldn't read NV variables. (%d)\n", ret);
	return 0;
    }
    
    /* Check to see if we need to update idme vars */
    if (strncmp((char *) (varbuf + (MMC_BLOCK_SIZE - CONFIG_IDME_UPDATE_MAGIC_SIZE)), 
		CONFIG_IDME_UPDATE_MAGIC,
		CONFIG_IDME_UPDATE_MAGIC_SIZE) == 0) {
	
	printf("Found updated idme variables.  Flashing...\n");
	
	while (len < CONFIG_MMC_USERDATA_SIZE) {

	    if (mmc_switch_partition(mmc, CONFIG_BOOT_FROM_PARTITION, 0) < 0) {
		printf("%s ERROR: couldn't switch to boot partition\n", __FUNCTION__);
		return -1;
	    }

	    ret = mmc_write(CONFIG_MMC_BOOTFLASH, varbuf, CONFIG_MMC_USERDATA_ADDR + len, MMC_BLOCK_SIZE);
	    if (ret) {
		printf("Error! Couldn't write NV variables. (%d)\n", ret);
		/* need to switch back to user partition even on error */
	    }

	    if (mmc_switch_partition(mmc, 0, 0) < 0 || ret) {
		printf("%s ERROR: couldn't switch back to user partition\n", __FUNCTION__);
		return -1;
	    }

	    /* clear out the idme vars so we only do this update once */
	    memset(varbuf, 0, MMC_BLOCK_SIZE);
	
	    ret = mmc_write(CONFIG_MMC_BOOTFLASH, varbuf, CONFIG_IDME_UPDATE_ADDR + len, MMC_BLOCK_SIZE);
	    if (ret) {
		printf("Error! Couldn't clear update variables. (%d)\n", ret);
		return -1;
	    }

	    len += MMC_BLOCK_SIZE;

	    ret = mmc_read(CONFIG_MMC_BOOTFLASH, CONFIG_IDME_UPDATE_ADDR + len, varbuf, MMC_BLOCK_SIZE);
	    if (ret) {
		printf("Error! Couldn't read NV variables. (%d)\n", ret);
		return 0;
	    }
	}

	printf("idme variable flash complete\n");
    }    
#endif /* CONFIG_IDME_UPDATE */

    return ret;
}

int do_idme (cmd_tbl_t *cmdtp, int flag, int argc, char *argv[])
{
    if (argc == 1 || strncmp(argv[1], "?", 1) == 0) {
	return idme_print_vars();
    } else if (argc <= 3) {
	char *value; 

	if (argc == 3) {
	    value = argv[2];
	    printf("setting '%s' to '%s'\n", argv[1], argv[2]);
	} else {
	    value = NULL;
	    printf("clearing '%s'\n", argv[1]);
	}

	return idme_update_var(argv[1], value);
    }

    printf ("Usage:\n%s", cmdtp->usage);
    return 1;	
}

/***************************************************/

U_BOOT_CMD(
	idme,	3,	1,	do_idme,
	"idme    - Set nv ram variables\n",
	"idme <var> <value>\n"
	"    - set a variable value\n"
	"idme ?"
	"    - print out known values\n"
);
