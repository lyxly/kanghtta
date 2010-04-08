/*
 * volume_id - reads filesystem label and uuid
 *
 * Copyright (C) 2006 Kay Sievers <kay.sievers@vrfy.org>
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

struct jmicron_meta {
	int8_t		signature[2];
	uint8_t		minor_version;
	uint8_t		major_version;
	uint16_t	checksum;
} PACKED;

int volume_id_probe_jmicron_raid(struct volume_id *id, uint64_t off, uint64_t size)
{
	const uint8_t *buf;
	uint64_t meta_off;
	struct jmicron_meta *jm;

	info("probing at offset 0x%llx, size 0x%llx",
	    (unsigned long long) off, (unsigned long long) size);

	if (size < 0x10000)
		return -1;

	meta_off = ((size / 0x200)-1) * 0x200;
	buf = volume_id_get_buffer(id, off + meta_off, 0x200);
	if (buf == NULL)
		return -1;

	jm = (struct jmicron_meta *) buf;
	if (memcmp((char *)jm->signature, "JM", 2) != 0)
		return -1;

	volume_id_set_usage(id, VOLUME_ID_RAID);
	sprintf(id->type_version, "%u.%u", jm->major_version, jm->minor_version);
	id->type = "jmicron_raid_member";

	return 0;
}
