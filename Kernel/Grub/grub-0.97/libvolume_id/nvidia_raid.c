/*
 * volume_id - reads filesystem label and uuid
 *
 * Copyright (C) 2005 Kay Sievers <kay.sievers@vrfy.org>
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

#include "volume_id.h"
#include "util.h"
#include "shared.h"

struct nvidia_meta {
	uint8_t		vendor[8];
	uint32_t	size;
	uint32_t	chksum;
	uint16_t	version;
} PACKED;

#define NVIDIA_SIGNATURE		"NVIDIA"

int volume_id_probe_nvidia_raid(struct volume_id *id, uint64_t off, uint64_t size)
{
	const uint8_t *buf;
	uint64_t meta_off;
	struct nvidia_meta *nv;

	info("probing at offset 0x%llx, size 0x%llx",
	    (unsigned long long) off, (unsigned long long) size);

	if (size < 0x10000)
		return -1;

	meta_off = ((size / 0x200)-2) * 0x200;
	buf = volume_id_get_buffer(id, off + meta_off, 0x200);
	if (buf == NULL)
		return -1;

	nv = (struct nvidia_meta *) buf;
	if (memcmp((char *)nv->vendor, NVIDIA_SIGNATURE, sizeof(NVIDIA_SIGNATURE)-1) != 0)
		return -1;

	volume_id_set_usage(id, VOLUME_ID_RAID);
	sprintf(id->type_version, "%u", le16_to_cpu(nv->version));
	id->type = "nvidia_raid_member";

	return 0;
}
