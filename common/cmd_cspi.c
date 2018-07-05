/*
 * cmd_cspi.c 
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
#ifdef CONFIG_MX50_CSPI

#include <command.h>
#ifdef CONFIG_CMD_CSPI

#include <asm/arch/mx50_pins.h>
#include <asm/arch/iomux.h>
#include "mx50_cspi.h"

#define DPRINTF(args...)
//#define DPRINTF(args...)  printf(args)

#define MXC_PMIC_FRAME_MASK		0x00FFFFFF
#define MXC_PMIC_MAX_REG_NUM	0x3F
#define MXC_PMIC_REG_NUM_SHIFT	0x19
#define MXC_PMIC_WRITE_BIT_SHIFT	31

int port_idx = 0;

int az_pmic_chip_init(void) {

    ecspi_init(port_idx);

    return 1;
}

int az_pmic_chip_write_reg(int reg, const unsigned int val) 
{
    int ret;
    unsigned int frame = 0;

    frame |= (1 << MXC_PMIC_WRITE_BIT_SHIFT);
    frame |= reg << MXC_PMIC_REG_NUM_SHIFT;
    frame |= val & MXC_PMIC_FRAME_MASK;

    ret = ecspi_write(frame);
    if (!ret)
	return ret;

    /* discard read result */
    ecspi_read(&frame);

    return 1;
}

int az_pmic_chip_read_reg(int reg, unsigned int *val) 
{
    int ret;
    unsigned int frame = 0;
    
    *val = 0;

    frame |= reg << MXC_PMIC_REG_NUM_SHIFT;
    ret = ecspi_write(frame);
    if (!ret) 
	return ret;

    ret = ecspi_read(&frame);
    *val = frame & MXC_PMIC_FRAME_MASK;

    return 1;
}

static int do_ecspi_rtc(cmd_tbl_t *cmdtp, int flag, int argc, char *argv[])
{
	int j;
	uint buf =0;
	az_pmic_chip_init();
	for (j = 0; j < 4; j++)
	{
		az_pmic_chip_read_reg(20 + j, &buf);
		printf("(addr, data)=(%02d, %08X)\n", 20 + j, buf);
	}
}

static int do_ecspi_demo(cmd_tbl_t *cmdtp, int flag, int argc, char *argv[])
{
	int j;
	uint buf =0;
	az_pmic_chip_init();
	for (j = 0; j < 4; j++)
	{
		az_pmic_chip_write_reg(20 + j, buf);
		printf("(addr, data)=(%02d, %08X)\n", 20 + j, buf);
	}
	for (j = 0; j < 4; j++)
	{
		az_pmic_chip_read_reg(20 + j, &buf);
		printf("(addr, data)=(%02d, %08X)\n", 20 + j, buf);
	}
}

static int do_ecspi(cmd_tbl_t * cmdtp, int flag, int argc, char *argv[])
{
	/* Strip off leading 'ecspi' command argument */
	argc--;
	argv++;

	DPRINTF("arg: %d, %s\n", argc, argv[0]);

	if (!strncmp(argv[0], "po", 2))
	{
		argc--;
		argv++;
		DPRINTF("arg: %d, %s\n", argc, argv[0]);
		if (argc > 0)
		{
			port_idx = simple_strtoul(argv[0], NULL, 10);
		}
		printf("port selected: %d\n", port_idx);
		return 0;
	}

	if (!strncmp(argv[0], "de", 2))
	{
		return do_ecspi_demo(cmdtp, flag, argc, argv);
	}

	if (!strncmp(argv[0], "rt", 2))
	{
		return do_ecspi_rtc(cmdtp, flag, argc, argv);
	}

	cmd_usage(cmdtp);
	return 0;
}

/***************************************************/

U_BOOT_CMD(
	ecspi, 7, 1, do_ecspi,
	"ECSPI sub-system",
	"\n"
	"ecspi port [dev] - show/set ESCPI port\n"
	"ecspi demo - demo\n"
	"ecspi rtc - rela time clock test"
);
#endif	//CONFIG_CMD_CSPI
#endif
