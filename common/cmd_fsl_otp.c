/*
 * (C) Copyright 2008-2010 Freescale Semiconductor, Inc.
 *
 * Copyright 2007, Freescale Semiconductor, Inc
 * Andy Fleming
 *
 * Based vaguely on the pxa mmc code:
 * (C) Copyright 2003
 * Kyle Harris, Nexus Technologies, Inc. kharris@nexus-tech.net
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

#include <linux/types.h>
#include <command.h>
#include <common.h>
#include <fsl_otp.h>

int do_ocotp(cmd_tbl_t *cmdtp, int flag, int argc, char *argv[])
{
    unsigned int index, val;

    if (argc < 2 || argc > 4)
	goto err_rtn;

    if (strcmp(argv[1], "read") == 0) {
	if (3 == argc) {

	    index = simple_strtoul(argv[2], NULL, 16);
		
	    if (!fsl_otp_show(index, &val)) {
		printf("Fuse %d: 0x%x\n", index, val);
	    } else {
		printf("Error reading fuse!\n");
	    }
	} else {
	    goto err_rtn;
	}

    } else if (strcmp(argv[1], "blow") == 0) {
	if (4 == argc) {
	    index = simple_strtoul(argv[2], NULL, 16);
	    val = simple_strtoul(argv[3], NULL, 16);

	    printf("Blowing fuse %d: 0x%x\n", index, val);
	    if (fsl_otp_store(index, val)) {
		printf("Error blowing fuse!\n");
	    }

	} else {
	    goto err_rtn;
	}
    } else {
	goto err_rtn;
    }

    return 0;
  err_rtn:
    printf("Invalid parameters!\n");
    return 1;
}

U_BOOT_CMD(
	   ocotp, 5, 1, do_ocotp,
	   "OCOTP sub system",
	   "Warning: all numbers in parameter are in hex format!\n"
	   "ocotp read <fuse>	- Read a fuse\n"
	   "ocotp blow <fuse> <value>	- Blow a fuse\n");
