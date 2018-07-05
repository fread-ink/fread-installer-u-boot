/*
 * (C) Copyright 2001
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
 * Misc functions
 */
#include <common.h>
#include <command.h>

int do_sleep (cmd_tbl_t *cmdtp, int flag, int argc, char *argv[])
{
	ulong start = get_timer(0);
	ulong delay;

	if (argc != 2) {
		cmd_usage(cmdtp);
		return 1;
	}

	delay = simple_strtoul(argv[1], NULL, 10) * CONFIG_SYS_HZ;

	while (get_timer(start) < delay) {
		if (ctrlc ()) {
			return (-1);
		}
		udelay (100);
	}

	return 0;
}

/* Implemented in $(CPU)/interrupts.c */
#if defined(CONFIG_CMD_IRQ)
int do_irqinfo (cmd_tbl_t *cmdtp, int flag, int argc, char *argv[]);

U_BOOT_CMD(
	irqinfo,    1,    1,     do_irqinfo,
	"print information about IRQs",
	""
);
#endif

#ifdef CONFIG_CMD_HALT
extern void board_power_off(void);
int do_halt (cmd_tbl_t *cmdtp, int flag, int argc, char *argv[])
{
    board_power_off();
    return 0;
}

U_BOOT_CMD(
    halt,   1,      1,  do_halt,
    "halt board",
);
#endif

#ifdef CONFIG_CMD_PANIC
int do_panic (cmd_tbl_t *cmdtp, int flag, int argc, char *argv[])
{
    panic ("Can't continue");
    return 0;
}

U_BOOT_CMD(
    panic,  1,      1,  do_panic,
    "panic halt",
);
#endif

#ifdef CONFIG_CMD_FAIL
extern int board_pmic_init(void);
extern void sos (void);
extern void ok (void);

int do_pass (cmd_tbl_t *cmdtp, int flag, int argc, char *argv[])
{
    board_pmic_init();
    while (1) {
        ok();
    }
}

U_BOOT_CMD(
    pass,  1,      1,  do_pass,
    "pass blink pass pattern on LED",
);

int do_fail (cmd_tbl_t *cmdtp, int flag, int argc, char *argv[])
{
    board_pmic_init();
    while (1) {
        sos();
    }
}

U_BOOT_CMD(
    fail,  1,      1,  do_fail,
    "fail blink fail pattern on LED",
);
#endif

U_BOOT_CMD(
	sleep ,    2,    1,     do_sleep,
	"delay execution for some time",
	"N\n"
	"    - delay execution for N seconds (N is _decimal_ !!!)"
);
