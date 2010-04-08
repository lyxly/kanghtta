/*
 * volume_id - reads filesystem label and uuid
 *
 * Copyright (C) 2004 Kay Sievers <kay.sievers@vrfy.org>
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

#define VXFS_SUPER_MAGIC	0xa501FCF5

struct vxfs_super {
	uint32_t		vs_magic;
	int32_t			vs_version;
} PACKED;

int volume_id_probe_vxfs(struct volume_id *id, uint64_t off, uint64_t size)
{
	struct vxfs_super *vxs;

	info("probing at offset 0x%llx", (unsigned long long) off);

	vxs = (struct vxfs_super *) volume_id_get_buffer(id, off + 0x200, 0x200);
	if (vxs == NULL)
		return -1;

	if (vxs->vs_magic == cpu_to_le32(VXFS_SUPER_MAGIC)) {
		sprintf(id->type_version, "%u", (unsigned int) vxs->vs_version);
		volume_id_set_usage(id, VOLUME_ID_FILESYSTEM);
		id->type = "vxfs";
		return 0;
	}

	return -1;
}
