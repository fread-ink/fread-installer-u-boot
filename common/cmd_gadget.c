/*
 * cmd_gadget.c
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

#if 0
#ifdef CONFIG_GADGET_FASTBOOT
#include <usb/fastboot.h>

int do_fastboot (cmd_tbl_t *cmdtp, int flag, int argc, char *argv[])
{
    int dev, part;

    if (argc > 2) {
	part = (int) simple_strtoul(argv[2], NULL, 10);
    } else {
	part = FASTBOOT_USE_DEFAULT;
    }

    if (argc > 1) {
	dev = (int) simple_strtoul(argv[1], NULL, 10);
    } else {
	dev = CONFIG_MMC_BOOTFLASH;
    }

    return fastboot_enable(dev, part);
}

U_BOOT_CMD(
	fastboot, 3, 1, do_fastboot,
	"Fastboot",
	"fastboot");
#endif
#endif

#ifdef CONFIG_GADGET_FILE_STORAGE
#include <usb/file_storage.h>

int do_file_storage (cmd_tbl_t *cmdtp, int flag, int argc, char *argv[])
{
    return file_storage_enable(5000);
}

U_BOOT_CMD(
	fstor, 1, 1, do_file_storage,
	"File Storage",
	"fstor");
#endif
