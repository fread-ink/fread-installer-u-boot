/*
 * (C) Copyright 2002
 * Sysgo Real-Time Solutions, GmbH <www.elinos.com>
 * Marius Groeger <mgroeger@sysgo.de>
 *
 * Copyright (C) 2001  Erik Mouw (J.A.K.Mouw@its.tudelft.nl)
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.	 See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307	 USA
 *
 */

#include <common.h>
#include <command.h>
#include <image.h>
#include <u-boot/zlib.h>
#include <asm/byteorder.h>

DECLARE_GLOBAL_DATA_PTR;

#if defined (CONFIG_SETUP_MEMORY_TAGS) || \
    defined (CONFIG_CMDLINE_TAG) || \
    defined (CONFIG_INITRD_TAG) || \
    defined (CONFIG_SERIAL_TAG) || \
    defined (CONFIG_REVISION_TAG) || \
    defined (CONFIG_POST_TAG) || \
    defined (CONFIG_VFD) || \
    defined (CONFIG_LCD)
static void setup_start_tag (bd_t *bd);

# ifdef CONFIG_SETUP_MEMORY_TAGS
static void setup_memory_tags (bd_t *bd);
# endif
static void setup_commandline_tag (bd_t *bd, char *commandline);

# ifdef CONFIG_INITRD_TAG
static void setup_initrd_tag (bd_t *bd, ulong initrd_start,
			      ulong initrd_end);
# endif
static void setup_end_tag (bd_t *bd);

#ifdef CONFIG_POST_TAG
void setup_post_tag (struct tag **params);
#endif

#ifdef CONFIG_MACADDR_TAG
void setup_macaddr_tag (struct tag **params);
#endif

# if defined (CONFIG_VFD) || defined (CONFIG_LCD)
static void setup_videolfb_tag (gd_t *gd);
# endif

static struct tag *params;
#endif /* CONFIG_SETUP_MEMORY_TAGS || CONFIG_CMDLINE_TAG || CONFIG_INITRD_TAG */

int do_bootm_linux(int flag, int argc, char *argv[], bootm_headers_t *images)
{
	bd_t	*bd = gd->bd;
	char	*s;
	int	machid = bd->bi_arch_number;
	void	(*theKernel)(int zero, int arch, uint params);

#ifdef CONFIG_CMDLINE_TAG
	char *commandline = getenv ("bootargs");
#endif

	if ((flag != 0) && (flag != BOOTM_STATE_OS_GO))
		return 1;

	theKernel = (void (*)(int, int, uint))images->ep;

	s = getenv ("machid");
	if (s) {
		machid = simple_strtoul (s, NULL, 16);
		printf ("Using machid 0x%x from environment\n", machid);
	}

	show_boot_progress (15);

	debug ("## Transferring control to Linux (at address %08lx) ...\n",
	       (ulong) theKernel);

#if defined (CONFIG_SETUP_MEMORY_TAGS) || \
    defined (CONFIG_CMDLINE_TAG) || \
    defined (CONFIG_INITRD_TAG) || \
    defined (CONFIG_SERIAL_TAG) || \
    defined (CONFIG_REVISION_TAG) || \
    defined (CONFIG_POST_TAG) || \
    defined (CONFIG_LCD) || \
    defined (CONFIG_VFD)
	setup_start_tag (bd);
#ifdef CONFIG_SERIAL_TAG
	setup_serial_tag (&params);
#endif
#ifdef CONFIG_SERIAL16_TAG
	setup_serial16_tag (&params);
#endif
#ifdef CONFIG_SERIAL20_TAG
	setup_serial20_tag (&params);
#endif
#ifdef CONFIG_BOOTMODE_TAG
	setup_bootmode_tag (&params);
#endif
#ifdef CONFIG_REVISION_TAG
	setup_revision_tag (&params);
#endif
#ifdef CONFIG_REVISION16_TAG
	setup_revision16_tag (&params);
#endif
#ifdef CONFIG_MACADDR_TAG
	setup_macaddr_tag (&params);
#endif
#ifdef CONFIG_POST_TAG
	setup_post_tag (&params);
#endif
#ifdef CONFIG_SETUP_MEMORY_TAGS
	setup_memory_tags (bd);
#endif
#ifdef CONFIG_CMDLINE_TAG
	setup_commandline_tag (bd, commandline);
#endif
#ifdef CONFIG_INITRD_TAG
	if (images->rd_start && images->rd_end)
		setup_initrd_tag (bd, images->rd_start, images->rd_end);
#endif
#if defined (CONFIG_VFD) || defined (CONFIG_LCD)
	setup_videolfb_tag ((gd_t *) gd);
#endif
	setup_end_tag (bd);
#endif

	/* we assume that the kernel is in place */
	printf ("Starting kernel ...\n");

#ifdef CONFIG_USB_DEVICE
	{
		extern void udc_disconnect (void);
		udc_disconnect ();
	}
#endif

	cleanup_before_linux ();

	theKernel (0, machid, bd->bi_boot_params);
	/* does not return */

	return 1;
}


#if defined (CONFIG_SETUP_MEMORY_TAGS) || \
    defined (CONFIG_CMDLINE_TAG) || \
    defined (CONFIG_INITRD_TAG) || \
    defined (CONFIG_SERIAL_TAG) || \
    defined (CONFIG_SERIAL16_TAG) || \
    defined (CONFIG_SERIAL20_TAG) || \
    defined (CONFIG_BOOTMODE_TAG) || \
    defined (CONFIG_REVISION_TAG) || \
    defined (CONFIG_REVISION16_TAG) || \
    defined (CONFIG_POST_TAG) || \
    defined (CONFIG_LCD) || \
    defined (CONFIG_VFD)
static void setup_start_tag (bd_t *bd)
{
	params = (struct tag *) bd->bi_boot_params;

	params->hdr.tag = ATAG_CORE;
	params->hdr.size = tag_size (tag_core);

	params->u.core.flags = 0;
	params->u.core.pagesize = 0;
	params->u.core.rootdev = 0;

	params = tag_next (params);
}


#ifdef CONFIG_SETUP_MEMORY_TAGS
static void setup_memory_tags (bd_t *bd)
{
	int i;

	for (i = 0; i < CONFIG_NR_DRAM_BANKS; i++) {
		params->hdr.tag = ATAG_MEM;
		params->hdr.size = tag_size (tag_mem32);

		params->u.mem.start = bd->bi_dram[i].start;
		params->u.mem.size = bd->bi_dram[i].size;

		params = tag_next (params);
	}
}
#endif /* CONFIG_SETUP_MEMORY_TAGS */


static void setup_commandline_tag (bd_t *bd, char *commandline)
{
	char *p;

	if (!commandline)
		return;

	/* eat leading white space */
	for (p = commandline; *p == ' '; p++);

	/* skip non-existent command lines so the kernel will still
	 * use its default command line.
	 */
	if (*p == '\0')
		return;

	params->hdr.tag = ATAG_CMDLINE;
	params->hdr.size =
		(sizeof (struct tag_header) + strlen (p) + 1 + 4) >> 2;

	strcpy (params->u.cmdline.cmdline, p);

	params = tag_next (params);
}


#ifdef CONFIG_INITRD_TAG
static void setup_initrd_tag (bd_t *bd, ulong initrd_start, ulong initrd_end)
{
	/* an ATAG_INITRD node tells the kernel where the compressed
	 * ramdisk can be found. ATAG_RDIMG is a better name, actually.
	 */
	params->hdr.tag = ATAG_INITRD2;
	params->hdr.size = tag_size (tag_initrd);

	params->u.initrd.start = initrd_start;
	params->u.initrd.size = initrd_end - initrd_start;

	params = tag_next (params);
}
#endif /* CONFIG_INITRD_TAG */


#if defined (CONFIG_VFD) || defined (CONFIG_LCD)
extern ulong calc_fbsize (void);
static void setup_videolfb_tag (gd_t *gd)
{
	/* An ATAG_VIDEOLFB node tells the kernel where and how large
	 * the framebuffer for video was allocated (among other things).
	 * Note that a _physical_ address is passed !
	 *
	 * We only use it to pass the address and size, the other entries
	 * in the tag_videolfb are not of interest.
	 */
	params->hdr.tag = ATAG_VIDEOLFB;
	params->hdr.size = tag_size (tag_videolfb);

	params->u.videolfb.lfb_base = (u32) gd->fb_base;
	/* Fb size is calculated according to parameters for our panel
	 */
	params->u.videolfb.lfb_size = calc_fbsize();

	params = tag_next (params);
}
#endif /* CONFIG_VFD || CONFIG_LCD */

#ifdef CONFIG_SERIAL_TAG
void setup_serial_tag (struct tag **tmp)
{
	struct tag *params = *tmp;
	struct tag_serialnr serialnr;
	void get_board_serial(struct tag_serialnr *serialnr);

	get_board_serial(&serialnr);
	params->hdr.tag = ATAG_SERIAL;
	params->hdr.size = tag_size (tag_serialnr);
	params->u.serialnr.low = serialnr.low;
	params->u.serialnr.high= serialnr.high;
	params = tag_next (params);
	*tmp = params;
}
#endif

#ifdef CONFIG_SERIAL16_TAG
extern const u8 *get_board_serial(void);

/* 16-byte serial number tag (alphanumeric string) */
void setup_serial16_tag(struct tag **in_params)
{
	const u8 *sn = 0;

	sn = get_board_serial();
	if (!sn)
		return; /* ignore if NULL was returned. */
	params->hdr.tag = ATAG_SERIAL16;
	params->hdr.size = tag_size (tag_id16);
	memcpy(params->u.id16.data, sn, sizeof params->u.id16.data);
	params = tag_next (params);
}
#endif  /* CONFIG_SERIAL16_TAG */

#ifdef CONFIG_SERIAL20_TAG
extern const u8 *get_board_serial(void);

/* 20-byte serial number tag (alphanumeric string) */
void setup_serial20_tag(struct tag **in_params)
{
	const u8 *sn = 0;

	sn = get_board_serial();
	if (!sn)
		return; /* ignore if NULL was returned. */
	params->hdr.tag = ATAG_SERIAL20;
	params->hdr.size = tag_size (tag_id20);
	memcpy(params->u.id20.data, sn, sizeof params->u.id20.data);
	params = tag_next (params);
}
#endif  /* CONFIG_SERIAL20_TAG */

#ifdef CONFIG_REVISION_TAG
void setup_revision_tag(struct tag **in_params)
{
	u32 rev = 0;
	u32 get_board_rev(void);

	rev = get_board_rev();
	params->hdr.tag = ATAG_REVISION;
	params->hdr.size = tag_size (tag_revision);
	params->u.revision.rev = rev;
	params = tag_next (params);
}
#endif  /* CONFIG_REVISION_TAG */

#ifdef CONFIG_REVISION16_TAG
extern const u8 *get_board_id16(void);

/* 16-byte revision tag (alphanumeric string) */
void setup_revision16_tag(struct tag **in_params)
{
	const u8 *rev = 0;

	rev = get_board_id16();
	if (!rev)
		return; /* ignore if NULL was returned. */
	params->hdr.tag = ATAG_REVISION16;
	params->hdr.size = tag_size (tag_id16);
	memcpy (params->u.id16.data, rev, sizeof params->u.id16.data);
	params = tag_next (params);
}
#endif  /* CONFIG_REVISION16_TAG */

#ifdef CONFIG_MACADDR_TAG
#ifdef CONFIG_CMD_IDME
#include <idme.h>
#endif

/* MAC address/secret tag (alphanumeric strings) */
void setup_macaddr_tag(struct tag **in_params)
{
#ifdef CONFIG_CMD_IDME    
    char mac_buf[12+1];
    char sec_buf[20+1];
    
    if (idme_get_var("mac", mac_buf, sizeof(mac_buf)))
	return;

    if (idme_get_var("sec", sec_buf, sizeof(sec_buf)))
	return;

    params->hdr.tag = ATAG_MACADDR;
    params->hdr.size = tag_size (tag_macaddr);
    memcpy (params->u.macaddr.address, mac_buf, sizeof params->u.macaddr.address);
    memcpy (params->u.macaddr.secret, sec_buf, sizeof params->u.macaddr.secret);
	
    params = tag_next (params);
#endif

}
#endif  /* CONFIG_MACADDR_TAG */

#ifdef CONFIG_BOOTMODE_TAG
#ifdef CONFIG_CMD_IDME
#include <idme.h>
#endif

/* bootmode tag (alphanumeric strings) */
void setup_bootmode_tag(struct tag **in_params)
{
#ifdef CONFIG_CMD_IDME    
    char bootmode_buf[16+1];
    char postmode_buf[16+1];
    
    if (idme_get_var("bootmode", bootmode_buf, sizeof(bootmode_buf)))
	return;

    if (idme_get_var("postmode", postmode_buf, sizeof(postmode_buf)))
	return;

    params->hdr.tag = ATAG_BOOTMODE;
    params->hdr.size = tag_size (tag_bootmode);
    memcpy (params->u.bootmode.boot, bootmode_buf, sizeof params->u.bootmode.boot);
    memcpy (params->u.bootmode.post, postmode_buf, sizeof params->u.bootmode.post);
	
    params = tag_next (params);
#endif

}
#endif  /* CONFIG_BOOTMODE_TAG */

#ifdef CONFIG_POST_TAG
void setup_post_tag (struct tag **in_params)
{
	params->hdr.tag = ATAG_POST;
	params->hdr.size = tag_size (tag_post);
	if (gd->flags & GD_FLG_POSTWARN) {
	    params->u.post.failure = 1;
	} else {
	    params->u.post.failure = 0;
	}
	params = tag_next (params);
}
#endif  /* CONFIG_POST_TAG */

static void setup_end_tag (bd_t *bd)
{
	params->hdr.tag = ATAG_NONE;
	params->hdr.size = 0;
}

#endif /* CONFIG_SETUP_MEMORY_TAGS || CONFIG_CMDLINE_TAG || CONFIG_INITRD_TAG */
