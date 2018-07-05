/*
 * (C) Copyright 2003
 * Kyle Harris, kharris@nexus-tech.net
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
#include <mmc.h>

#ifndef CONFIG_GENERIC_MMC
static int curr_device = -1;

int do_mmc (cmd_tbl_t *cmdtp, int flag, int argc, char *argv[])
{
	int dev;
	unsigned int mem_addr;
	unsigned int mmc_addr;
	int length;
	ulong st, end;
	int err;

	if (argc < 2) {
		cmd_usage(cmdtp);
		return 1;
	}

	if (strcmp(argv[1], "init") == 0) {
		if (argc == 2) {
			if (curr_device < 0)
				dev = 1;
			else
				dev = curr_device;
		} else if (argc == 3) {
			dev = (int)simple_strtoul(argv[2], NULL, 10);
		} else {
			cmd_usage(cmdtp);
			return 1;
		}

		if (mmc_legacy_init(dev) != 0) {
			puts("No MMC card found\n");
			return 1;
		}

		curr_device = dev;
		printf("mmc%d is available\n", curr_device);
	} else if (strcmp(argv[1], "device") == 0) {
		if (argc == 2) {
			if (curr_device < 0) {
				puts("No MMC device available\n");
				return 1;
			}
		} else if (argc == 3) {
			dev = (int)simple_strtoul(argv[2], NULL, 10);

#ifdef CONFIG_SYS_MMC_SET_DEV
			if (mmc_set_dev(dev) != 0)
				return 1;
#endif
			curr_device = dev;
		} else {
			cmd_usage(cmdtp);
			return 1;
		}

		printf("mmc%d is current device\n", curr_device);
	} else if (strcmp(argv[1], "read") == 0) {

	    if (argc != 5) {
		printf("argc=%d\n", argc);

		cmd_usage(cmdtp);
		return 1;
	    }
	    
	    mem_addr = (unsigned int)simple_strtoul(argv[2], NULL, 10);
	    mmc_addr = (unsigned int)simple_strtoul(argv[3], NULL, 10);
	    length = (int)simple_strtoul(argv[4], NULL, 10);
	    
	    printf("read from 0x%x to 0x%x, len=%d bytes\n", mmc_addr, mem_addr, length); 

	    st = get_timer_masked();
	    err = mmc_read(curr_device, mmc_addr, (unsigned char *) mem_addr, length);
	    if (!err) {
		end = get_timer_masked();
		printf("read %d bytes in %ld msec\n", length, ((end - st) * 1000) / CONFIG_SYS_HZ);
	    } else {
		printf("read error: %d\n", err);
	    }
	} else if (strcmp(argv[1], "write") == 0) {

	    if (argc != 5) {
		printf("argc=%d\n", argc);
		cmd_usage(cmdtp);
		return 1;
	    }
	    
	    mem_addr = (unsigned int)simple_strtoul(argv[2], NULL, 10);
	    mmc_addr = (unsigned int)simple_strtoul(argv[3], NULL, 10);
	    length = (int)simple_strtoul(argv[4], NULL, 10);
	    
	    printf("write from 0x%x to 0x%x, len=%d bytes\n", mem_addr, mmc_addr, length); 

	    st = get_timer_masked();
	    err = mmc_write(curr_device, (unsigned char *) mem_addr, mmc_addr, length);
	    if (!err) {
		end = get_timer_masked();
		printf("wrote %d bytes in %ld msec\n", length, ((end - st) * 1000) / CONFIG_SYS_HZ);
	    } else {
		printf("write error: %d\n", err);
	    }
	} else {
		cmd_usage(cmdtp);
		return 1;
	}

	return 0;
}

U_BOOT_CMD(
	mmc, 5, 1, do_mmc,
	"MMC sub-system",
	"init [dev] - init MMC sub system\n"
	"mmc device [dev] - show or set current device"
);
#else /* !CONFIG_GENERIC_MMC */

#ifdef CONFIG_BOOT_PARTITION_ACCESS
#define MMC_PARTITION_SWITCH(mmc, part, enable_boot) \
	do { \
		if (IS_SD(mmc)) {	\
			if (part > 1)	{\
				printf( \
				"\nError: SD partition can only be 0 or 1\n");\
				return 1;	\
			}	\
			if (sd_switch_partition(mmc, part) < 0) {	\
				if (part > 0) { \
					printf("\nError: Unable to switch SD "\
					"partition\n");\
					return 1;	\
				}	\
			}	\
		} else {	\
			if (mmc_switch_partition(mmc, part, enable_boot) \
				< 0) {	\
				printf("Error: Fail to switch "	\
					"partition to %d\n", part);	\
				return 1;	\
			}	\
		} \
	} while (0)
#endif

static void print_mmcinfo(struct mmc *mmc)
{
	printf("Device: %s\n", mmc->name);
	printf("Manufacturer ID: %x\n", mmc->cid[0] >> 24);
	printf("OEM: %x\n", (mmc->cid[0] >> 8) & 0xffff);
	printf("Name: %c%c%c%c%c%c \n", mmc->cid[0] & 0xff,
	       (mmc->cid[1] >> 24), (mmc->cid[1] >> 16) & 0xff,
	       (mmc->cid[1] >> 8) & 0xff, mmc->cid[1] & 0xff, (mmc->cid[2] >> 24));
	printf("Revision: 0x%x\n", (mmc->cid[2] >> 16) & 0xff);
	printf("Tran Speed: %d\n", mmc->tran_speed);
	printf("Rd Block Len: %d\n", mmc->read_bl_len);

	printf("%s version %d.%d\n", IS_SD(mmc) ? "SD" : "MMC",
			(mmc->version >> 4) & 0xf, mmc->version & 0xf);

	printf("High Capacity: %s\n", mmc->high_capacity ? "Yes" : "No");
	printf("Capacity: %lld\n", mmc->capacity);

	printf("Bus Width: %d-bit\n", mmc->bus_width);
#ifdef CONFIG_BOOT_PARTITION_ACCESS
	if (mmc->boot_size_mult == 0) {
		printf("Boot Partition Size: %s\n", "No boot partition available");
	} else {
		printf("Boot Partition Size: %5dKB\n", mmc->boot_size_mult * 128);

		printf("Current Partition for boot: ");
		switch (mmc->boot_config & EXT_CSD_BOOT_PARTITION_ENABLE_MASK) {
		case EXT_CSD_BOOT_PARTITION_DISABLE:
			printf("Not bootable\n");
			break;
		case EXT_CSD_BOOT_PARTITION_PART1:
			printf("Boot partition 1\n");
			break;
		case EXT_CSD_BOOT_PARTITION_PART2:
			printf("Boot partition 2\n");
			break;
		case EXT_CSD_BOOT_PARTITION_USER:
			printf("User area\n");
			break;
		default:
			printf("Unknown\n");
			break;
		}

		printf("Current boot width: ");
		switch (mmc->boot_bus_width & 0x3) {
		  case EXT_CSD_BOOT_BUS_WIDTH_1BIT:
		    printf("1-bit ");
		    break;
		  case EXT_CSD_BOOT_BUS_WIDTH_4BIT:
		    printf("4-bit ");
		    break;
		  case EXT_CSD_BOOT_BUS_WIDTH_8BIT:
		    printf("8-bit ");
		  default:
		    printf("Unknown ");
		}

		switch ((mmc->boot_bus_width >> 3) & 0x3) {
		  case 0:
		    printf("SDR\n");
		    break;
		  case EXT_CSD_BOOT_BUS_WIDTH_SDR_HS:
		    printf("SDR High-speed\n");
		    break;
		  case EXT_CSD_BOOT_BUS_WIDTH_DDR:
		    printf("DDR\n");
		  default:
		    printf("Unknown\n");
		}
	}
#endif
}

int do_mmcinfo (cmd_tbl_t *cmdtp, int flag, int argc, char *argv[])
{
	struct mmc *mmc;
	int dev_num;

	if (argc < 2)
		dev_num = 0;
	else
		dev_num = simple_strtoul(argv[1], NULL, 0);

	mmc = find_mmc_device(dev_num);

	if (mmc) {
		if (mmc_init(mmc))
			puts("MMC card init failed!\n");
		else
			print_mmcinfo(mmc);
	}

	return 0;
}

U_BOOT_CMD(mmcinfo, 2, 0, do_mmcinfo,
	"mmcinfo <dev num>-- display MMC info",
	""
);

int do_mmcops(cmd_tbl_t *cmdtp, int flag, int argc, char *argv[])
{
	int rc = 0;
#ifdef CONFIG_BOOT_PARTITION_ACCESS
	u32 part = 0;
#endif
	int err = 0;
	ulong st, end, tm;

	switch (argc) {
#ifdef CONFIG_BOOT_PARTITION_ACCESS
	case 4:
	  part = simple_strtoul(argv[3], NULL, 10);
	  /* Fall through */
#endif
	case 3:
		if (strcmp(argv[1], "rescan") == 0) {
			int dev = simple_strtoul(argv[2], NULL, 10);
			struct mmc *mmc = find_mmc_device(dev);

			if (!mmc)
				return 1;

			mmc_init(mmc);

			return 0;
		} else if (!strcmp(argv[1], "mmctest")) {

		    int dev = simple_strtoul(argv[2], NULL, 10);
		    struct mmc *mmc = find_mmc_device(dev);
		    int num = 0;

		    while(1) {
			MMC_PARTITION_SWITCH(mmc, 1, 1);
			err = mmc_read(dev, 0, (unsigned char *) 0x70000000, 512);
			if (err)
			    printf("err on read!\n");
			MMC_PARTITION_SWITCH(mmc, 0, 0);
			num++;

			if (!(num % 1000))
			    printf(".");
			if (num > 30000) {
			    printf("\n");
			    num = 0;
			}
		    }

		    return 0;

		} else if (!strcmp(argv[1], "erase")) {

		    int dev = simple_strtoul(argv[2], NULL, 10);
		    struct mmc *mmc = find_mmc_device(dev);
		    int end_addr = mmc->capacity - 512;

#ifdef CONFIG_BOOT_PARTITION_ACCESS
		    if (((mmc->boot_config &
			  EXT_CSD_BOOT_PARTITION_ACCESS_MASK) != part)
			|| IS_SD(mmc)) {
			/*
			 * After mmc_init, we now know whether
				 * this is a eSD/eMMC which support boot
				 * partition
				 */
			MMC_PARTITION_SWITCH(mmc, part, 0);
		    }

		    /* Only erase first erase group for non-user partition */
		    if (part != 0)
			end_addr = 0;
#endif
		    err = mmc_erase(mmc, 0, end_addr);
		    if (err) {
			printf("MMC erase failed: %d!\n", err);
			return 1;
		    }

		    return 0;

		}
#if defined(CONFIG_CMD_MMC_SAMSUNG_UPGRADE) 
		/* Code to field-upgrade Samsung eMMC firmware
		 * New firmware must be loaded at address 0x7A00000 in memory
		 * This should only be run from a bist loaded by the mfgtool as 
		 * it will completely wipe the flash part 
		 */
		else if (!strcmp(argv[1], "upgrade")) {

		    int dev = simple_strtoul(argv[2], NULL, 10);
		    struct mmc *mmc = find_mmc_device(dev);
		    struct mmc_cmd cmd;

		    /* Check for Samsung devices */
		    if ((mmc->cid[0] >> 24) != 0x15) {
			printf("Can only upgrade Samsung devices\n");
			return 1;
		    }

		    printf("Entering factory program mode..\n");

		    /* Go into factory mode */
		    memset(&cmd, 0, sizeof(cmd));
		    cmd.cmdidx = MMC_CMD_VENDOR;
		    cmd.resp_type = MMC_RSP_R1b;
		    cmd.cmdarg = 0xEFAC60FC;
		    err = mmc->send_cmd(mmc, &cmd, NULL);
		    if (err) {
			printf("Vendor command 1 failed: %d! Resetting..\n", err);
			goto done;
		    }

		    memset(&cmd, 0, sizeof(cmd));
		    cmd.cmdidx = MMC_CMD_VENDOR;
		    cmd.resp_type = MMC_RSP_R1b;
		    cmd.cmdarg = 0xCBAD1160;
		    err = mmc->send_cmd(mmc, &cmd, NULL);
		    if (err) {
			printf("Vendor command 2 failed: %d! Resetting..\n", err);
			goto done;
		    }

		    printf("Erasing flash..\n");

		    err = mmc_erase(mmc);
		    if (err) {
			printf("MMC erase failed: %d! Resetting..\n", err);
			goto done;
		    }
		    
#define SAMSUNG_PATCH_LEN	(128*1024)
#define DOWNLOAD_ADDR		CONFIG_FASTBOOT_TEMP_BUFFER
#define READBACK_ADDR		0x70000000

		    printf("Writing new firmware..\n");

		    /* Write binary - 128k from temp storage area */
		    err = mmc_write(dev, (u8 *) DOWNLOAD_ADDR, 0, SAMSUNG_PATCH_LEN);
		    if (err) {
			printf("Patch write failed: %d! Resetting..\n", err);
			goto done;
		    }

		    printf("Verifying firmware..\n");

		    /* Read binary back and verify */
		    err = mmc_read(dev, 0, (u8 *) READBACK_ADDR, SAMSUNG_PATCH_LEN);
		    if (err) {
			printf("Patch readback failed: %d! Resetting..\n", err);
			goto done;
		    }

		    if (memcmp((u8 *) DOWNLOAD_ADDR, (u8 *) READBACK_ADDR, SAMSUNG_PATCH_LEN)) {
			printf("Patch confirm failed! Resetting..\n");
			goto done;
		    }

		    printf("Finalizing upgrade..\n");

		    /* Confirm firmware */
		    memset(&cmd, 0, sizeof(cmd));
		    cmd.cmdidx = MMC_CMD_SET_WRITE_PROT;
		    cmd.resp_type = MMC_RSP_R1b;
		    err = mmc->send_cmd(mmc, &cmd, NULL);
		    if (err) {
			printf("Patch confirm command failed: %d! Resetting..\n", err);
			goto done;
		    }		    

		    /* Reset device */
		    printf("Done.\n");
		  done:
		    board_reset();

		    return 0;
		}
#endif

		return 1;

	case 0:
	case 1:
		printf("Usage:\n%s\n", cmdtp->usage);
		return 1;

	case 2:
		if (!strcmp(argv[1], "list")) {
			print_mmc_devices('\n');
			return 0;
		}
		return 1;
#ifdef CONFIG_BOOT_PARTITION_ACCESS
	case 7: /* Fall through */
		part = simple_strtoul(argv[6], NULL, 10);
#endif
	default: /* at least 5 args */
		if (strcmp(argv[1], "read") == 0) {
			int dev = simple_strtoul(argv[2], NULL, 10);
			void *addr = (void *)simple_strtoul(argv[3], NULL, 16);
			u32 blk = simple_strtoul(argv[4], NULL, 16);
			u32 cnt = simple_strtoul(argv[5], NULL, 16);
			u32 len, read, rdlen;

			struct mmc *mmc = find_mmc_device(dev);
			if (!mmc)
			    return 1;

			err = mmc_init(mmc);
			if (err) {
			    printf("MMC read error initing mmc %d\n", dev);
			    return err;
			}


#ifdef CONFIG_BOOT_PARTITION_ACCESS
			printf("\nMMC read: dev # %d, block # %d, "
				"count %d partition # %d ... \n",
				dev, blk, cnt, part);
#else
			printf("\nMMC read: dev # %d, flashaddr=0x%x, size %d ... ",
			       dev, blk, (cnt * mmc->read_bl_len));
#endif

#ifdef CONFIG_BOOT_PARTITION_ACCESS
			if (((mmc->boot_config &
				EXT_CSD_BOOT_PARTITION_ACCESS_MASK) != part)
				|| IS_SD(mmc)) {
				/*
				 * After mmc_init, we now know whether
				 * this is a eSD/eMMC which support boot
				 * partition
				 */
				MMC_PARTITION_SWITCH(mmc, part, 0);
			}
#endif

			st = get_timer_masked();

			len = cnt * mmc->read_bl_len;
			read = 0;
			// 64k transfers are optimal
			while (len && !err) {
			    rdlen = MIN(CONFIG_MMC_MAX_TRANSFER_SIZE, len); 
			    err = mmc_read(dev, blk + read, addr + read, rdlen);
			    read += rdlen;
			    len -= rdlen;
			}
			
			if (!err) {
			    end = get_timer_masked();

			    tm = (end - st) / 1000;
			    printf("read %d blocks in %ld msec\n", cnt, tm);

#if 0 // BEN DEBUG
			    {
				int i;
				
				for (i = 0; i < read; i++) {
				    if (!(i%16)) printf("\n%08x: ", addr + i);
				    printf("%02x ", ((u8 *) addr)[i]);
				}
				printf("\n");
			    }
#endif
			} else {
			    printf("read error: %d\n", err);
			}

			

			return err;

		} else if (strcmp(argv[1], "write") == 0) {
			int dev = simple_strtoul(argv[2], NULL, 10);
			void *addr = (void *)simple_strtoul(argv[3], NULL, 16);
			u32 blk = simple_strtoul(argv[4], NULL, 16);
			u32 cnt = simple_strtoul(argv[5], NULL, 16);
			u32 len, written, wrlen;

			struct mmc *mmc = find_mmc_device(dev);
			if (!mmc)
			    return 1;

			err = mmc_init(mmc);
			if (err) {
			    printf("MMC write error initing mmc %d\n", dev);
			    return err;
			}

#ifdef CONFIG_BOOT_PARTITION_ACCESS
			printf("\nMMC write: dev # %d, block # %d, "
				"count %d, partition # %d ... \n",
				dev, blk, cnt, part);
#else
		printf("\nMMC write: dev # %d, flashaddr=0x%x, size %d ... ",
			       dev, blk, (cnt * mmc->write_bl_len));
#endif

#ifdef CONFIG_BOOT_PARTITION_ACCESS
			if (((mmc->boot_config &
				EXT_CSD_BOOT_PARTITION_ACCESS_MASK) != part)
				|| IS_SD(mmc)) {
				/*
				 * After mmc_init, we now know whether this is a
				 * eSD/eMMC which support boot partition
				 */
				MMC_PARTITION_SWITCH(mmc, part, 1);
			}
#endif

			st = get_timer_masked();

			len = cnt * mmc->write_bl_len;
			written = 0;
			// 64k transfers are optimal
			while (len && !err) {
			    wrlen = MIN(CONFIG_MMC_MAX_TRANSFER_SIZE, len); 
			    err = mmc_write(dev, addr + written, blk + written, wrlen);
			    written += wrlen;
			    len -= wrlen;
			}
			
			if (!err) {
			    end = get_timer_masked();

			    tm = (end - st) / 1000;
			    printf("wrote %d blocks in %ld msec\n", cnt, tm);
			} else {
			    printf("write error: %d\n", err);
			}

			return err;

		} else {
			printf("Usage:\n%s\n", cmdtp->usage);
			rc = 1;
		}

		return rc;
	}
}

#ifndef CONFIG_BOOT_PARTITION_ACCESS
U_BOOT_CMD(
	mmc, 7, 1, do_mmcops,
	"MMC sub system",
	"mmc read <device num> addr blk# cnt\n"
	"mmc write <device num> addr blk# cnt\n"
	"mmc rescan <device num>\n"
	"mmc list - lists available devices");
#else
U_BOOT_CMD(
	mmc, 7, 1, do_mmcops,
	"MMC sub system",
	"mmc read <device num> addr blk# cnt [partition]\n"
	"mmc write <device num> addr blk# cnt [partition]\n"
	"mmc rescan <device num>\n"
	"mmc list - lists available devices");
#endif
#endif
