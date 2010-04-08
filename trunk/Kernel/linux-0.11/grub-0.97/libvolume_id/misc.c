/*
 * volume_id/misc.c
 *
 * Copyright (C) 2008 Canonical Ltd.
 *
 *      This program is free software; you can redistribute it and/or modify it
 *      under the terms of the GNU General Public License as published by the
 *      Free Software Foundation version 2 of the License.
 */
#define _GNU_SOURCE
#include <string.h>

/*
 *  Misc auxiliary functions required for volume_id inside grub
 */
size_t strnlen(const char *s, size_t limit)
{
	size_t length = 0;
	while ( (length < limit) && (*s++) ) 
		length++;

   	return length;
}

char *strchr (const char *s, int c)
{
	do {
		if ( *s == c ) {
			return (char*)s;
      		}
  	} while ( *s++ );

  	return 0;
}
