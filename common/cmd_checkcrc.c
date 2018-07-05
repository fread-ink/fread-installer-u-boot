/*
 * cmd_checkcrc.c 
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
#include <post.h>

extern int mmc_crc32_test (uint part, uint start, int size, uint crc);

int do_check_crc (cmd_tbl_t * cmdtp, int flag, int argc, char *argv[])
{
	int size, i, part = -1;
	uint start, crc;

	debug("argc=[%d], argv=[0,%s],[1,%s],[2,%s],[3,%s],[4,%s],flag=0x%08x\n",
		argc, argv[0], argv[1], argv[2], argv[3], argv[4], flag);

	if (argc == 5) {
		for (i = 0; i < CONFIG_NUM_PARTITIONS; i++) {

			if (strcmp(argv[1], partition_info[i].name) == 0) {
				part = i;
				break;
			} else debug("%s <> %s\n", argv[1], partition_info[i].name);
		}

		if (part < 0) {
#if defined(CONFIG_MX35)
			printf("image must be bootloader, kernel or system\n");
#else
			printf("image must be %s", partition_info[0].name);
			for (i = 1; i < CONFIG_NUM_PARTITIONS; i++) {
				printf(", %s", partition_info[i].name);
			}
			printf("\n");
#endif
			return -1;
		}

		start = (uint)simple_strtoul(argv[2], NULL, 16);
		size = (uint)simple_strtoul(argv[3], NULL, 16);
		crc = (uint)simple_strtoul(argv[4], NULL, 16);

		debug("crc32 test, part=0x%08x, start=0x%08x, size=0x%08x, crc=0x%08x\n", part, start, size, crc);

		return mmc_crc32_test(part, start, size, crc);
	} else {
		printf("usage: check image start size crc\n");
		return -1;
	}
}
/***************************************************/

U_BOOT_CMD(
	check,	5,	0,	do_check_crc,
	"perform MMC CRC32 check",
	"image start size crc\n"
	"    - images:\n"
	"    - bootloader comes from u-boot.bin\n"
	"    - kernel comes from uImage\n"
	"    - system comes from rootfs.img"
);

