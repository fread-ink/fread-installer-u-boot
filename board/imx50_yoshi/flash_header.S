/*
 * Copyright (C) 2010 Freescale Semiconductor, Inc.
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

#include <config.h>
#include <asm/arch/mx50.h>

#ifdef	CONFIG_FLASH_HEADER
#ifndef CONFIG_FLASH_HEADER_OFFSET
# error "Must define the offset of flash header"
#endif

.section ".text.flasheader", "x"
	b	_start

/*=============================================================================
 * Here starts the plugin code
 *===========================================================================*/

plugin_start:

/* Save the return address and the function arguments */
	push    {r0-r6, r8, lr}

/* To return to ROM from plugin, we need to fill in these argument.
 * Here is what need to do:
 * Need to construct the paramters for this function before return to ROM:
 * plugin_download(void **start, size_t *bytes, UINT32 *ivt_offset)
 */
 /* Check r0 if valid address
  * Mfgtools or sb_loader run-plug maybe pass down invalidate data
 */
 	cmp r0, #0xf8000000
	mov r4, r0
 	ldr r3, DDR_DEST_ADDR
	strhi r3, [r0]
	ldr r3, COPY_SIZE
	strhi r3, [r1]
	ldr r3, IVT2_HDR_OFS
	strhi r3, [r2]

/*=============================================================================
 *init script for codex LPDDR1-200MHz CPU board
 *===========================================================================*/

/* Setup PLL1 to be 800 MHz */
	ldr r0, =CCM_BASE_ADDR

/* Switch ARM domain to be clocked from LP-APM */
	mov r1, #0x4
	str r1, [r0, #CLKCTL_CCSR]

	ldr r0, =PLL1_BASE_ADDR


	/*
	 * PLL1 workaround as the following
	 *      (1) Disable AREN bit to avoid PLL1 restart during MFN change
	 *      (2) set PLL1 to ~864Mhz with MFI = 8, MFN = 180, MFD = 179, PDF = 0
	 *      (3) Manual restart PLL1
	 *      (4) Wait PLL1 lock
	 *      (5) Set PLL1 to 800Mhz with only change MFN to 60, others keep
	 *      (6) Set LDREQ bit to load new MFN
	 *      (7) Poll on LDREQ bit for MFN update to be completed
	 *      (8) Delay at least 4 us to avoid PLL1 instability window
	 *      (9) Switch ARM back to PLL1
	 */
	/* Step 1 * /
	ldr r1, [r0, #PLL_DP_CONFIG]
	bic r1, r1, #0x2
	str r1, [r0, #PLL_DP_CONFIG] /* disable auto-restart AREN bit */

	/* Step 2 */
	ldr   r1, =0x80
	str   r1, [r0, #PLL_DP_OP]
	str   r1, [r0, #PLL_DP_HFS_OP]

	ldr   r1, =180
	str   r1, [r0, #PLL_DP_MFN]
	str   r1, [r0, #PLL_DP_HFS_MFN]

	ldr   r1, =179
	str   r1, [r0, #PLL_DP_MFD]
	str   r1, [r0, #PLL_DP_HFS_MFD]

	/* Step 3. */
	ldr    r1, =0x00001236 /* Set PLM =1, manual restart and enable PLL*/
	str   r1, [r0, #PLL_DP_CTL]

	/* Step 4 */
1:    ldr    r1, [r0, #0x0]

	ands r1, r1, #0x1
	
	beq   1b

	/* Step 5 -  set PLL1 to 800Mhz */
	ldr    r1, =60
	str   r1, [r0, #PLL_DP_MFN]
	str   r1, [r0, #PLL_DP_HFS_MFN]

	/* Step 6 - Set the LDREQ bit */
	ldr    r1, [r0, #PLL_DP_CONFIG]
	orr    r1, r1, #1
	str    r1, [r0, #PLL_DP_CONFIG]

	/* Step 7 - Poll until LDREQ clears */
2:    ldr    r1, [r0, #PLL_DP_CONFIG]
	tst    r1, #0x1
	bne   2b

	/* Step 8 - delay more than 4 us */
	mov   r1, #100
3:   subs   r1, r1, #1
	bge    3b
 
	/* Step 9 - Switch ARM back to PLL1 */
	ldr r0, =CCM_BASE_ADDR
	ldr r1, =0x0
	str r1, [r0,#CLKCTL_CCSR]

/*=============================================================================
 * Enable all clocks (they are disabled by ROM code)
 *===========================================================================*/

	mov r1, #0xffffffff
	str r1, [r0, #0x68]
	str r1, [r0, #0x6c]
	str r1, [r0, #0x70]
	str r1, [r0, #0x74]
	str r1, [r0, #0x78]
	str r1, [r0, #0x7c]
	str r1, [r0, #0x80]
	str r1, [r0, #0x84]

#ifndef CONFIG_IRAM_BOOT
#if defined(CONFIG_LPDDR2)
	bl lpddr2_init
#elif defined(CONFIG_MDDR)
	bl mddr_init
#endif
#endif

/*
 * The following is to fill in those arguments for this ROM function
 * pu_irom_hwcnfg_setup(void **start, size_t *bytes, const void *boot_data)
 *
 * This function is used to copy data from the storage media into DDR.

 * start - Initial (possibly partial) image load address on entry.
 * Final image load address on exit.
 * bytes - Initial (possibly partial) image size on entry.
 * Final image size on exit.
 * boot_data - Initial @ref ivt Boot Data load address.
 */

	ldr r0, =ROM_SI_REV
	ldr r1, [r0]
	cmp r1, #0x11

	adr r0, DDR_DEST_ADDR
	adr r1, COPY_SIZE
	adr r2, BOOT_DATA

before_calling_rom___pu_irom_hwcnfg_setup:
	mov r3, #0x2a00
	/* Different ROM address for TO 1.0 & TO 1.1.1 */
	addeq r3, r3, #0xad
	addne r3, r3, #0x19
	cmp r4, #0xf8000000
	blxhi r3
after_calling_rom___pu_irom_hwcnfg_setup:

	pop {r0-r6, r8, lr}
	mov r0, #1
	bx lr          /* return back to ROM code */

/* Data be copied by pu_irom_hwcnfg_setup() */
DDR_DEST_ADDR:		.word TEXT_BASE
COPY_SIZE:		.word _end - TEXT_BASE
BOOT_DATA:		.word TEXT_BASE
			.word _end - TEXT_BASE
			.word 0
IVT2_HDR_OFS:           .word ivt2_header - TEXT_BASE

	.org	CONFIG_FLASH_HEADER_OFFSET

/* First IVT to copy the plugin that initializes the system into OCRAM */
ivt_header:        .long 0x402000D1    /* Tag=0xD1, Len=0x0020, Ver=0x40 */
app_code_jump_v:   .long 0xF8006004    /* Plugin entry point */
reserv1:           .long 0x0
dcd_ptr:           .long 0x0
boot_data_ptr:     .long 0xF8006420
self_ptr:          .long 0xF8006400
app_code_csf:      .long 0x0
reserv2:           .long 0x0
boot_data:         .long 0xF8006000
image_len:         .long 2*1024        /* Can copy upto 72K, OCRAM free space */
plugin:            .long 0x1  	       /* Enable plugin flag */

/* Second IVT to give entry point into the bootloader copied to DDR */
ivt2_header:       .long 0x402000D1    /* Tag=0xD1, Len=0x0020, Ver=0x40 */
app2_code_jump_v:  .long _start   /* Entry point for the bootloader */
reserv3:           .long 0x0
dcd2_ptr:          .long 0x0
boot_data2_ptr:    .long boot_data2
self_ptr2:         .long ivt2_header
app_code_csf2:     .long 0x0
reserv4:           .long 0x0
boot_data2:        .long TEXT_BASE
image_len2:        .long _end - TEXT_BASE
plugin2:           .long 0x0
	
#endif
