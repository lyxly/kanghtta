/*
 * volume_id - reads filesystem label and uuid
 *
 * Copyright (C) 2004 Kay Sievers <kay.sievers@vrfy.org>
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

struct hpt37x_meta {
	uint8_t		filler1[32];
	uint32_t	magic;
} PACKED;

struct hpt45x_meta {
	uint32_t	magic;
} PACKED;

#define HPT37X_CONFIG_OFF		0x1200
#define HPT37X_MAGIC_OK			0x5a7816f0
#define HPT37X_MAGIC_BAD		0x5a7816fd

#define HPT45X_MAGIC_OK			0x5a7816f3
#define HPT45X_MAGIC_BAD		0x5a7816fd


int volume_id_probe_highpoint_37x_raid(struct volume_id *id, uint64_t off, uint64_t size)
{
	const uint8_t *buf;
	struct hpt37x_meta *hpt;
	uint32_t magic;

	info("probing at offset 0x%llx", (unsigned long long) off);

	buf = volume_id_get_buffer(id, off + HPT37X_CONFIG_OFF, 0x200);
	if (buf == NULL)
		return -1;

	hpt = (struct hpt37x_meta *) buf;
	magic = le32_to_cpu(hpt->magic);
	if (magic != HPT37X_MAGIC_OK && magic != HPT37X_MAGIC_BAD)
		return -1;

	volume_id_set_usage(id, VOLUME_ID_RAID);
	id->type = "highpoint_raid_member";

	return 0;
}

int volume_id_probe_highpoint_45x_raid(struct volume_id *id, uint64_t off, uint64_t size)
{
	const uint8_t *buf;
	struct hpt45x_meta *hpt;
	uint64_t meta_off;
	uint32_t magic;

	dbg("probing at offset 0x%llx, size 0x%llx",
	    (unsigned long long) off, (unsigned long long) size);

	if (size < 0x10000)
		return -1;

	meta_off = ((size / 0x200)-11) * 0x200;
	buf = volume_id_get_buffer(id, off + meta_off, 0x200);
	if (buf == NULL)
		return -1;

	hpt = (struct hpt45x_meta *) buf;
	magic = le32_to_cpu(hpt->magic);
	if (magic != HPT45X_MAGIC_OK && magic != HPT45X_MAGIC_BAD)
		return -1;

	volume_id_set_usage(id, VOLUME_ID_RAID);
	id->type = "highpoint_raid_member";

	return 0;
}
