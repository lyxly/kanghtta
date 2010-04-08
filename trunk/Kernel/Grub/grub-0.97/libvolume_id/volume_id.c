/*
 * volume_id - reads volume label and uuid
 *
 * Copyright (C) 2005-2007 Kay Sievers <kay.sievers@vrfy.org>
 * Grub Port, Copyright (C) 2008 Canonical Ltd.
 *
 *	This program is free software; you can redistribute it and/or modify it
 *	under the terms of the GNU General Public License as published by the
 *	Free Software Foundation version 2 of the License.
 */

#ifndef _GNU_SOURCE
#define _GNU_SOURCE 1
#endif

#ifdef HAVE_CONFIG_H
#  include <config.h>
#endif

#include <fcntl.h>
#include <sys/stat.h>

#include "volume_id.h"
#include "util.h"
#include "strfuncs.h"
#include "shared.h"

#define ARRAY_SIZE(x) (sizeof(x) / sizeof((x)[0]))

struct prober {
	volume_id_probe_fn_t prober;
	const char *name[4];
};

#define NOT_SUPPORTED(x)	
#define SUPPORTED(x)	x

static const struct prober prober_raid[] = {
	{ volume_id_probe_ddf_raid, { "ddf_raid", } },

	/* { volume_id_probe_linux_raid, { "linux_raid", } },  */
	/* { volume_id_probe_intel_software_raid, { "isw_raid", } }, */
	/* { volume_id_probe_lsi_mega_raid, { "lsi_mega_raid", } }, */
	/* { volume_id_probe_via_raid, { "via_raid", } }, */
	/* { volume_id_probe_silicon_medley_raid, { "silicon_medley_raid", } }, */
	/* { volume_id_probe_nvidia_raid, { "nvidia_raid", } }, */
	/* { volume_id_probe_promise_fasttrack_raid, { "promise_fasttrack_raid", } }, */
	/* { volume_id_probe_highpoint_45x_raid, { "highpoint_raid", } }, */
	/* { volume_id_probe_adaptec_raid, { "adaptec_raid", } }, */
	/* { volume_id_probe_jmicron_raid, { "jmicron_raid", } }, */
	/* { volume_id_probe_lvm1, { "lvm1", } }, */
	/* { volume_id_probe_lvm2, { "lvm2", } }, */
	/* { volume_id_probe_highpoint_37x_raid, { "highpoint_raid", } }, */
};

static const struct prober prober_filesystem[] = {
	{ volume_id_probe_vfat, { "vfat", } }, 
	{ volume_id_probe_luks, { "luks", } }, 
	{ volume_id_probe_xfs, { "xfs", } }, 
	{ volume_id_probe_ext, { "ext2", "ext3", "jbd", } }, 
	{ volume_id_probe_reiserfs, { "reiserfs", "reiser4", } }, 
	{ volume_id_probe_jfs, { "jfs", } }, 
	{ volume_id_probe_hfs_hfsplus, { "hfs", "hfsplus", } }, 
	{ volume_id_probe_ntfs, { "ntfs", } },
 	{ volume_id_probe_ocfs1, { "ocfs1", } },
	{ volume_id_probe_ocfs2, { "ocfs2", } }, 

	/* { volume_id_probe_linux_swap, { "swap", } }, */
	/* { volume_id_probe_udf, { "udf", } }, */
	/* { volume_id_probe_iso9660, { "iso9660", } }, */
	/* { volume_id_probe_ufs, { "ufs", } },  */
	/* { volume_id_probe_cramfs, { "cramfs", } }, */
	/* { volume_id_probe_romfs, { "romfs", } }, */
	/* { volume_id_probe_hpfs, { "hpfs", } }, */
	/* { volume_id_probe_sysv, { "sysv", "xenix", } }, */
	/* { volume_id_probe_minix, { "minix",  } }, */
	/* { volume_id_probe_vxfs, { "vxfs", } }, */
	/* { volume_id_probe_squashfs, { "squashfs", } }, */
	/* { volume_id_probe_netware, { "netware", } }, */
};

/* the user can overwrite this log function */
static void default_log(int priority, const char *file, int line, const char *format, ...)
{
	return;
}

volume_id_log_fn_t volume_id_log_fn = default_log;

/**
 * volume_id_get_type:
 * @id: Probing context
 * @type: Type string. Must not be freed by the caller.
 *
 * Get the type string after a successful probe.
 *
 * Returns: 1 if the value was set, 0 otherwise.
 **/
int volume_id_get_type(struct volume_id *id, const char **type)
{
	if (id == NULL)
		return 0;
	if (type == NULL)
		return 0;
	if (id->usage_id == VOLUME_ID_UNUSED)
		return 0;

	*type = id->type;
	return 1;
}


/**
 * volume_id_get_uuid:
 * @id: Probing context.
 * @uuid: UUID string. Must not be freed by the caller.
 *
 * Get the raw UUID string after a successful probe.
 *
 * Returns: 1 if the value was set, 0 otherwise.
 **/
int volume_id_get_uuid(struct volume_id *id, const char **uuid)
{
	if (id == NULL)
		return 0;
	if (uuid == NULL)
		return 0;
	if (id->usage_id == VOLUME_ID_UNUSED)
		return 0;

	*uuid = id->uuid;
	return 1;
}

/**
 * volume_id_probe_raid:
 * @id: Probing context.
 * @off: Probing offset relative to the start of the device.
 * @size: Total size of the device.
 *
 * Probe device for all known raid signatures.
 *
 * Returns: 0 on successful probe, otherwise negative value.
 **/
int volume_id_probe_raid(struct volume_id *id, uint64_t off, uint64_t size)
{
	unsigned int i;

	if (id == NULL)
		return -1;

	info("probing at offset 0x%llx, size 0x%llx",
	    (unsigned long long) off, (unsigned long long) size);

	for (i = 0; i < ARRAY_SIZE(prober_raid); i++)
		if (prober_raid[i].prober(id, off, size) == 0)
			goto found;
	return -1;

found:
	return 0;
}

/**
 * volume_id_probe_filesystem:
 * @id: Probing context.
 * @off: Probing offset relative to the start of the device.
 * @size: Total size of the device.
 *
 * Probe device for all known filesystem signatures.
 *
 * Returns: 0 on successful probe, otherwise negative value.
 **/
int volume_id_probe_filesystem(struct volume_id *id, uint64_t off, uint64_t size)
{
	unsigned int i;

	if (id == NULL)
		return -1;

	info("probing at offset 0x%llx, size 0x%llx",
	    (unsigned long long) off, (unsigned long long) size);

	for (i = 0; i < ARRAY_SIZE(prober_filesystem); i++)
		if (prober_filesystem[i].prober(id, off, size) == 0)
			goto found;
	return -1;

found:
	return 0;
}

/**
 * volume_id_probe_raid:
 * @all_probers_fn: prober function to called for all known probing routines.
 * @id: Context passed to prober function.
 * @off: Offset value passed to prober function.
 * @size: Size value passed to prober function.
 * @data: Arbitrary data passed to the prober function.
 *
 * Run a custom function for all known probing routines.
 **/
void volume_id_all_probers(all_probers_fn_t all_probers_fn,
			   struct volume_id *id, uint64_t off, uint64_t size,
			   void *data)
{
	unsigned int i;

	if (all_probers_fn == NULL)
		return;

	for (i = 0; i < ARRAY_SIZE(prober_raid); i++) {	
		if (all_probers_fn(prober_raid[i].prober, id, off, size, data) != 0)
			goto out;
	}
	for (i = 0; i < ARRAY_SIZE(prober_filesystem); i++) {
		if (all_probers_fn(prober_filesystem[i].prober, id, off, size, data) != 0)
			goto out;
	}
out:
	return;
}
