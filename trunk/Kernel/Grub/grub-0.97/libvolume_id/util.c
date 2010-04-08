/*
 * volume_id - reads filesystem label and uuid
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

#include <stdlib.h>

#include "strfuncs.h"
#include "volume_id.h"
#include "util.h"
#include "shared.h"

static char hex[] = "0123456789abcdef";

#define hexhi(val)   hex[val >> 4]
#define hexlo(val)   hex[val & 0xf]

/* count of characters used to encode one unicode char */
static int utf8_encoded_expected_len(const char *str)
{
	unsigned char c = (unsigned char)str[0];

	if (c < 0x80)
		return 1;
	if ((c & 0xe0) == 0xc0)
		return 2;
	if ((c & 0xf0) == 0xe0)
		return 3;
	if ((c & 0xf8) == 0xf0)
		return 4;
	if ((c & 0xfc) == 0xf8)
		return 5;
	if ((c & 0xfe) == 0xfc)
		return 6;
	return 0;
}

/* decode one unicode char */
static int utf8_encoded_to_unichar(const char *str)
{
	int unichar;
	int len;
	int i;

	len = utf8_encoded_expected_len(str);
	switch (len) {
	case 1:
		return (int)str[0];
	case 2:
		unichar = str[0] & 0x1f;
		break;
	case 3:
		unichar = (int)str[0] & 0x0f;
		break;
	case 4:
		unichar = (int)str[0] & 0x07;
		break;
	case 5:
		unichar = (int)str[0] & 0x03;
		break;
	case 6:
		unichar = (int)str[0] & 0x01;
		break;
	default:
		return -1;
	}

	for (i = 1; i < len; i++) {
		if (((int)str[i] & 0xc0) != 0x80)
			return -1;
		unichar <<= 6;
		unichar |= (int)str[i] & 0x3f;
	}

	return unichar;
}

/* expected size used to encode one unicode char */
static int utf8_unichar_to_encoded_len(int unichar)
{
	if (unichar < 0x80)
		return 1;
	if (unichar < 0x800)
		return 2;
	if (unichar < 0x10000)
		return 3;
	if (unichar < 0x200000)
		return 4;
	if (unichar < 0x4000000)
		return 5;
	return 6;
}

/* check if unicode char has a valid numeric range */
static int utf8_unichar_valid_range(int unichar)
{
	if (unichar > 0x10ffff)
		return 0;
	if ((unichar & 0xfffff800) == 0xd800)
		return 0;
	if ((unichar > 0xfdcf) && (unichar < 0xfdf0))
		return 0;
	if ((unichar & 0xffff) == 0xffff)
		return 0;
	return 1;
}

/* validate one encoded unicode char and return its length */
int volume_id_utf8_encoded_valid_unichar(const char *str)
{
	int len;
	int unichar;
	int i;

	len = utf8_encoded_expected_len(str);
	if (len == 0)
		return -1;

	/* ascii is valid */
	if (len == 1)
		return 1;

	/* check if expected encoded chars are available */
	for (i = 0; i < len; i++)
		if ((str[i] & 0x80) != 0x80)
			return -1;

	unichar = utf8_encoded_to_unichar(str);

	/* check if encoded length matches encoded value */
	if (utf8_unichar_to_encoded_len(unichar) != len)
		return -1;

	/* check if value has valid range */
	if (!utf8_unichar_valid_range(unichar))
		return -1;

	return len;
}

size_t volume_id_set_unicode16(uint8_t *str, size_t len, const uint8_t *buf, enum endian endianess, size_t count)
{
	size_t i, j;
	uint16_t c;

	j = 0;
	for (i = 0; i + 2 <= count; i += 2) {
		if (endianess == LE)
			c = (buf[i+1] << 8) | buf[i];
		else
			c = (buf[i] << 8) | buf[i+1];
		if (c == 0) {
			str[j] = '\0';
			break;
		} else if (c < 0x80) {
			if (j+1 >= len)
				break;
			str[j++] = (uint8_t) c;
		} else if (c < 0x800) {
			if (j+2 >= len)
				break;
			str[j++] = (uint8_t) (0xc0 | (c >> 6));
			str[j++] = (uint8_t) (0x80 | (c & 0x3f));
		} else {
			if (j+3 >= len)
				break;
			str[j++] = (uint8_t) (0xe0 | (c >> 12));
			str[j++] = (uint8_t) (0x80 | ((c >> 6) & 0x3f));
			str[j++] = (uint8_t) (0x80 | (c & 0x3f));
		}
	}
	str[j] = '\0';
	return j;
}

static char *usage_to_string(enum volume_id_usage usage_id)
{
	switch (usage_id) {
	case VOLUME_ID_FILESYSTEM:
		return "filesystem";
	case VOLUME_ID_OTHER:
		return "other";
	case VOLUME_ID_RAID:
		return "raid";
	case VOLUME_ID_DISKLABEL:
		return "disklabel";
	case VOLUME_ID_CRYPTO:
		return "crypto";
	case VOLUME_ID_UNPROBED:
		return "unprobed";
	case VOLUME_ID_UNUSED:
		return "unused";
	}
	return NULL;
}

void volume_id_set_usage(struct volume_id *id, enum volume_id_usage usage_id)
{
	id->usage_id = usage_id;
	id->usage = usage_to_string(usage_id);
}

void volume_id_set_label_raw(struct volume_id *id, const uint8_t *buf, size_t count)
{
	if (count > sizeof(id->label))
		count = sizeof(id->label);

	memcpy(id->label_raw, buf, count);
	id->label_raw_len = count;
}

void volume_id_set_label_string(struct volume_id *id, const uint8_t *buf, size_t count)
{
	size_t i;

	if (count >= sizeof(id->label))
		count = sizeof(id->label)-1;

	memcpy(id->label, buf, count);
	id->label[count] = '\0';

	/* remove trailing whitespace */
	i = strnlen(id->label, count);
	while (i--) {
		if (!isspace(id->label[i]))
			break;
	}
	id->label[i+1] = '\0';
}

void volume_id_set_label_unicode16(struct volume_id *id, const uint8_t *buf, enum endian endianess, size_t count)
{
	if (count >= sizeof(id->label))
		count = sizeof(id->label)-1;

	 volume_id_set_unicode16((uint8_t *)id->label, sizeof(id->label), buf, endianess, count);
}

void volume_id_set_uuid(struct volume_id *id, const uint8_t *buf, size_t len, enum uuid_format format)
{
	unsigned int i;
	unsigned int count = 0;
	char *uuid;

	if (len > sizeof(id->uuid_raw))
		len = sizeof(id->uuid_raw);

	switch(format) {
	case UUID_STRING:
		count = len;
		break;
	case UUID_HEX_STRING:
		count = len;
		break;
	case UUID_DOS:
		count = 4;
		break;
	case UUID_64BIT_LE:
	case UUID_64BIT_BE:
		count = 8;
		break;
	case UUID_DCE:
		count = 16;
		break;
	case UUID_FOURINT:
		count = 35;
		break;
	}
	memcpy(id->uuid_raw, buf, count);
	id->uuid_raw_len = count;

	/* if set, create string in the same format, the native platform uses */
	for (i = 0; i < count; i++)
		if (buf[i] != 0)
			goto set;
	return;

set:
	uuid = id->uuid;
	switch(format) {
	case UUID_DOS:
		*uuid++ = hexhi(buf[3]);
		*uuid++ = hexlo(buf[3]);
		*uuid++ = hexhi(buf[2]);
		*uuid++ = hexlo(buf[2]);
		*uuid++ = '-';
		*uuid++ = hexhi(buf[1]);
		*uuid++ = hexlo(buf[1]);
		*uuid++ = hexhi(buf[0]);
		*uuid++ = hexlo(buf[0]);
		*uuid = '\0';
		break;
	case UUID_64BIT_LE:
		*uuid++ = hexhi(buf[7]);
		*uuid++ = hexlo(buf[7]);
		*uuid++ = hexhi(buf[6]);
		*uuid++ = hexlo(buf[6]);
		*uuid++ = hexhi(buf[5]);
		*uuid++ = hexlo(buf[5]);
		*uuid++ = hexhi(buf[4]);
		*uuid++ = hexlo(buf[4]);
		*uuid++ = hexhi(buf[3]);
		*uuid++ = hexlo(buf[3]);
		*uuid++ = hexhi(buf[2]);
		*uuid++ = hexlo(buf[2]);
		*uuid++ = hexhi(buf[1]);
		*uuid++ = hexlo(buf[1]);
		*uuid++ = hexhi(buf[0]);
		*uuid++ = hexlo(buf[0]);
		*uuid = '\0';
		break;
	case UUID_64BIT_BE:
		*uuid++ = hexhi(buf[0]);
		*uuid++ = hexlo(buf[0]);
		*uuid++ = hexhi(buf[1]);
		*uuid++ = hexlo(buf[1]);
		*uuid++ = hexhi(buf[2]);
		*uuid++ = hexlo(buf[2]);
		*uuid++ = hexhi(buf[3]);
		*uuid++ = hexlo(buf[3]);
		*uuid++ = hexhi(buf[4]);
		*uuid++ = hexlo(buf[4]);
		*uuid++ = hexhi(buf[5]);
		*uuid++ = hexlo(buf[5]);
		*uuid++ = hexhi(buf[6]);
		*uuid++ = hexlo(buf[6]);
		*uuid++ = hexhi(buf[7]);
		*uuid++ = hexlo(buf[7]);
		*uuid = '\0';
		break;
	case UUID_DCE:
		*uuid++ = hexhi(buf[0]);
		*uuid++ = hexlo(buf[0]);
		*uuid++ = hexhi(buf[1]);
		*uuid++ = hexlo(buf[1]);
		*uuid++ = hexhi(buf[2]);
		*uuid++ = hexlo(buf[2]);
		*uuid++ = hexhi(buf[3]);
		*uuid++ = hexlo(buf[3]);
		*uuid++ = '-';
		*uuid++ = hexhi(buf[4]);
		*uuid++ = hexlo(buf[4]);
		*uuid++ = hexhi(buf[5]);
		*uuid++ = hexlo(buf[5]);
		*uuid++ = '-';
		*uuid++ = hexhi(buf[6]);
		*uuid++ = hexlo(buf[6]);
		*uuid++ = hexhi(buf[7]);
		*uuid++ = hexlo(buf[7]);
		*uuid++ = '-';
		*uuid++ = hexhi(buf[8]);
		*uuid++ = hexlo(buf[8]);
		*uuid++ = hexhi(buf[9]);
		*uuid++ = hexlo(buf[9]);
		*uuid++ = '-';
		*uuid++ = hexhi(buf[10]);
		*uuid++ = hexlo(buf[10]);
		*uuid++ = hexhi(buf[11]);
		*uuid++ = hexlo(buf[11]);
		*uuid++ = hexhi(buf[12]);
		*uuid++ = hexlo(buf[12]);
		*uuid++ = hexhi(buf[13]);
		*uuid++ = hexlo(buf[13]);
		*uuid++ = hexhi(buf[14]);
		*uuid++ = hexlo(buf[14]);
		*uuid++ = hexhi(buf[15]);
		*uuid++ = hexlo(buf[15]);
		*uuid = '\0';
		break;
	case UUID_HEX_STRING:
		/* translate A..F to a..f */
		memcpy(id->uuid, buf, count);
		for (i = 0; i < count; i++)
			if (id->uuid[i] >= 'A' && id->uuid[i] <= 'F')
				id->uuid[i] = (id->uuid[i] - 'A') + 'a';
		id->uuid[count] = '\0';
		break;
	case UUID_STRING:
		memcpy(id->uuid, buf, count);
		id->uuid[count] = '\0';
		break;
	case UUID_FOURINT:
		*uuid++ = hexhi(buf[0]);
		*uuid++ = hexlo(buf[0]);
		*uuid++ = hexhi(buf[1]);
		*uuid++ = hexlo(buf[1]);
		*uuid++ = hexhi(buf[2]);
		*uuid++ = hexlo(buf[2]);
		*uuid++ = hexhi(buf[3]);
		*uuid++ = hexlo(buf[3]);
		*uuid++ = ':';
		*uuid++ = hexhi(buf[4]);
		*uuid++ = hexlo(buf[4]);
		*uuid++ = hexhi(buf[5]);
		*uuid++ = hexlo(buf[5]);
		*uuid++ = hexhi(buf[6]);
		*uuid++ = hexlo(buf[6]);
		*uuid++ = hexhi(buf[7]);
		*uuid++ = hexlo(buf[7]);
		*uuid++ = ':';
		*uuid++ = hexhi(buf[8]);
		*uuid++ = hexlo(buf[8]);
		*uuid++ = hexhi(buf[9]);
		*uuid++ = hexlo(buf[9]);
		*uuid++ = hexhi(buf[10]);
		*uuid++ = hexlo(buf[10]);
		*uuid++ = hexhi(buf[11]);
		*uuid++ = hexlo(buf[11]);
		*uuid++ = ':';
		*uuid++ = hexhi(buf[12]);
		*uuid++ = hexlo(buf[12]);
		*uuid++ = hexhi(buf[13]);
		*uuid++ = hexlo(buf[13]);
		*uuid++ = hexhi(buf[14]);
		*uuid++ = hexlo(buf[14]);
		*uuid++ = hexhi(buf[15]);
		*uuid++ = hexlo(buf[15]);
		*uuid = '\0';
		break;
	default:
		*uuid = '\0';
		break;
	}
}

uint8_t *volume_id_get_buffer(struct volume_id *id, uint64_t off, size_t len)
{
	info("get buffer off 0x%llx(%llu), len 0x%zx", (unsigned long long) off, (unsigned long long) off, len);
	/* check if requested area fits in superblock buffer */
	if (off + len <= SB_BUFFER_SIZE) {
		/* check if we need to read */
		if ((off + len) > id->sbbuf_len) {
			if (devread (0, 0, off + len, (char*)id->sbbuf) == 0)
                        {
				return NULL;
                        }
			id->sbbuf_len = off + len;
		}
		return &(id->sbbuf[off]);
	} else {
		if (len > SEEK_BUFFER_SIZE) {
			dbg("seek buffer too small %d", SEEK_BUFFER_SIZE);
			return NULL;
		}

		/* check if we need to read */
		if ((off < id->seekbuf_off) || ((off + len) > (id->seekbuf_off + id->seekbuf_len))) {
			info("read seekbuf off:0x%llx len:0x%zx", (unsigned long long) off, len);
			if (devread(off >> 9, off & 0x1ff, len, (char*)id->seekbuf) == 0)
			{
				return NULL;
			}
			id->seekbuf_off = off;
			id->seekbuf_len = len;
		}
		return &(id->seekbuf[off - id->seekbuf_off]);
	}
}
