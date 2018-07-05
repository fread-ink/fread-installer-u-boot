/*
 * (C) Copyright 2000-2003
 * Wolfgang Denk, DENX Software Engineering, wd@denx.de.
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

/*
 * Misc boot support
 */
#include <common.h>
#include <command.h>
#include <net.h>
#include <mmc.h>

/* Allow ports to override the default behavior */
__attribute__((weak))
unsigned long do_go_exec (ulong (*entry)(int, char *[]), int argc, char *argv[])
{
	return entry (argc, argv);
}

int do_go (cmd_tbl_t *cmdtp, int flag, int argc, char *argv[])
{
	ulong	addr, rc;
	int     rcode = 0;

	if (argc < 2) {
		cmd_usage(cmdtp);
		return 1;
	}

	addr = simple_strtoul(argv[1], NULL, 16);

	printf ("## Starting application at 0x%08lX ...\n", addr);

	/*
	 * pass address parameter as argv[0] (aka command name),
	 * and all remaining args
	 */
	rc = do_go_exec ((void *)addr, argc - 1, argv + 1);
	if (rc != 0) rcode = 1;

	printf ("## Application terminated, rc = 0x%lX\n", rc);
	return rcode;
}

/* -------------------------------------------------------------------- */

U_BOOT_CMD(
	go, CONFIG_SYS_MAXARGS, 1,	do_go,
	"start application at address 'addr'",
	"addr [arg ...]\n    - start application at address 'addr'\n"
	"      passing 'arg' as arguments"
);

extern int do_reset (cmd_tbl_t *cmdtp, int flag, int argc, char *argv[]);

U_BOOT_CMD(
	reset, 1, 0,	do_reset,
	"Perform RESET of the CPU",
	""
);

#ifdef CONFIG_CMD_BIST
int do_bist (cmd_tbl_t *cmdtp, int flag, int argc, char *argv[])
{
	int err;
	int i, ret;
	char *cmd = (char *) CONFIG_BISTCMD_LOCATION;

#if defined(CONFIG_BOOT_PARTITION_ACCESS) && defined(CONFIG_BOOT_FROM_PARTITION)
	struct mmc *mmc;

	mmc = find_mmc_device(CONFIG_MMC_BOOTFLASH);
	if (mmc == NULL) {
	    printf("Error: Couldn't find flash device");
	    return -1;
	}
 
	if (mmc_switch_partition(mmc, CONFIG_BOOT_FROM_PARTITION, 0) < 0) {
	    printf("ERROR: couldn't switch to boot partition\n");
	    return -1;
	}
#endif
	err = mmc_read(CONFIG_MMC_BOOTFLASH, \
				   CONFIG_MMC_BIST_ADDR, \
				   (unsigned char *) CONFIG_BISTADDR, \
				   CONFIG_MMC_BIST_SIZE);
	if (err) {
		printf("ERROR: couldn't read bist image from flash address 0x%x\n", \
			   CONFIG_MMC_BIST_ADDR);
		return err;
	}

#if defined(CONFIG_BOOT_PARTITION_ACCESS) && defined(CONFIG_BOOT_FROM_PARTITION)
	if (mmc_switch_partition(mmc, 0, 0) < 0) {
	    printf("ERROR: couldn't switch back to user partition\n");
	}
#endif

	cmd[0] = CONFIG_BISTCMD_MAGIC;
	cmd[1] = 0;

	/* copy cmd arguments to saved mem location */
	cmd = &(cmd[1]);	    
	for (i = 1; i < argc; i++) {
	    ret = sprintf(cmd, "%s ", argv[i]);
	    if (ret > 0) {
		cmd += ret;
	    } else {
		break;
	    }
	}

	/* null terminate */
	cmd[0] = 0;

	err = do_go_exec ((void *)(CONFIG_BISTADDR), argc - 1, argv + 1);
	printf ("## Application terminated, rc = 0x%X\n", err);

	return err;
}

U_BOOT_CMD(
	bist, CONFIG_SYS_MAXARGS, 0,	do_bist,
	"start Built In Self Test",
	""
);
#endif
