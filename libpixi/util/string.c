/*
    pixi-tools: a set of software to interface with the Raspberry Pi
    and PiXi-200 hardware
    Copyright (C) 2013 Simon Cantrill

    pixi-tools is free software; you can redistribute it and/or
    modify it under the terms of the GNU Lesser General Public
    License as published by the Free Software Foundation; either
    version 2.1 of the License, or (at your option) any later version.

    This library is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
    Lesser General Public License for more details.

    You should have received a copy of the GNU Lesser General Public
    License along with this library; if not, write to the Free Software
    Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA
*/

#include <libpixi/util/string.h>
#include <libpixi/util/log.h>
#include <ctype.h>
#include <stdlib.h>
#include <errno.h>

bool pixi_strEndsWith (const char* haystack, const char* needle)
{
	size_t needleLen   = strlen (needle);
	size_t haystackLen = strlen (haystack);
	if (haystackLen < needleLen)
		return false;
	const char* end = haystack + needleLen - haystackLen;
	return 0 == strcmp (end, needle);
}

bool pixi_strEndsWithI (const char* haystack, const char* needle)
{
	size_t needleLen   = strlen (needle);
	size_t haystackLen = strlen (haystack);
	if (haystackLen < needleLen)
		return false;
	const char* end = haystack + needleLen - haystackLen;
	return 0 == strcasecmp (end, needle);
}

size_t pixi_strCopy (const char* src, char* dest, size_t destLength)
{
	const char* s = src;
	if (src && dest && destLength)
	{
		char* d           = dest;
		const char* limit = d + destLength - 1;

		for ( ; (*s && d < limit); d++, s++)
			*d = *s;
		*d = '\0';
	}
	return s - src;
}

long pixi_parseLong (const char* str)
{
	if (!str)
	{
		LIBPIXI_PRECONDITION_FAILURE("str is NULL");
		return 0;
	}
	errno = 0;
	long value = strtol (str, NULL, 0);
	if (errno)
		LIBPIXI_ERRNO_ERROR("Error converting [%s] to long [%ld]", str, value);
	return value;
}

size_t pixi_strlenRStrip (const char* str)
{
	const char* cur = str;
	const char* end = NULL;
	while (*cur)
	{
		if (isspace (*cur) && !end)
			end = cur;
		else
			end = NULL;
		cur++;
	}
	if (end)
		return end - str;
	return cur - str;
}

size_t pixi_strnlenRStrip (const char* str, size_t length)
{
	const char* limit = str + length;
	const char* cur = str;
	const char* end = NULL;
	while (cur < limit && *cur)
	{
		if (isspace (*cur) && !end)
			end = cur;
		else
			end = NULL;
		cur++;
	}
	if (end)
		return end - str;
	return cur - str;
}

void pixi_strRStrip (char* str)
{
	size_t end = pixi_strlenRStrip (str);
	str[end] = '\0';
}

char* pixi_strStrip (char* str)
{
	while (str && isspace (*str))
		str++;
	pixi_strRStrip (str);
	return str;
}

bool pixi_strGetProperty (char* text, char separator, Property* property)
{
	LIBPIXI_PRECONDITION_NOT_NULL(text);
	LIBPIXI_PRECONDITION_NOT_NULL(property);

	char* sep = strchr (text, separator);
	if (sep <= text)
		return false;

	*sep = '\0';
	pixi_strRStrip (text);
	property->key = text;
	property->value = pixi_strStrip (sep + 1);
	return true;
}
