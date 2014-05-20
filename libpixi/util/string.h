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

#ifndef libpixi_util_string_h__included
#define libpixi_util_string_h__included


#include <libpixi/common.h>
#include <string.h>
#include <strings.h>

LIBPIXI_BEGIN_DECLS

///@defgroup util_string libpixi string utilities
///@{

///	Return true iff the start of @a haystack exactly matches @a needle.
static inline bool pixi_strStartsWith (const char* haystack, const char* needle) {
	return 0 == strncmp (haystack, needle, strlen (needle));
}

///	Return true iff the start of @a haystack exactly matches @a needle (case insensitive).
static inline bool pixi_strStartsWithI (const char* haystack, const char* needle) {
	return 0 == strncasecmp (haystack, needle, strlen (needle));
}

///	Return true iff the end of @a haystack exactly matches @a needle.
bool pixi_strEndsWith (const char* haystack, const char* needle);

///	Return true iff the end of @a haystack exactly matches @a needle (case insensitive).
bool pixi_strEndsWithI (const char* haystack, const char* needle);

///	Copy nul-terminated string @c src to buffer @c dest.
///	Copies up to @c destLength - 1 characters and always adds a nul terminator
///	(unless @c destLength is zero).
///	Functionally equivalent to strlcpy (but arguments are in a different order).
size_t pixi_strCopy (const char* src, char* dest, size_t destLength);

///	Equivalent to strlen (@c str), but trailing whitespace is ignored
size_t pixi_strlenRStrip (const char* str);

///	Equivalent to strnlen (@c str), but trailing whitespace is ignored
size_t pixi_strnlenRStrip (const char* str, size_t length);

///	Terminate @c str immediately before any trailing whitespace.
void pixi_strRStrip (char* str);

///	Terminate @c str immediately before any trailing whitespace, and
///	return a pointer to the first non whitespace character.
char* pixi_strStrip (char* str);

///	Parse an integer as decimal, hex (0x prefix) or octal (0 prefix).
///	This wraps around strtol(), but has the convenience (and issues) of atoi().
long pixi_parseLong (const char* str);

///	Represents a string key/value pair.
typedef struct Property
{
	const char*   key;
	const char*   value;
} Property;

///	Extract a key/value pair from string @c text.
///	If @c separator is found in @c text, @c text will be modified,
///	the key and value fields of @c property will be set, and true
///	will be returned. Otherwise false is returned.
bool pixi_strGetProperty (char* text, char separator, Property* property);

///	Hex encode @c input to @c output.  Basically, subsitute non-printable
///	characters with a hex code.
///	@param input input string
///	@param inputSize length of input string
///	@param output buffer for output string
///	@param capacity of output buffer
///	@param prefix hex prefix character
///	@param printable characters that don't need conversion
///	@return number of input characters processed. If less than @c inputSize, output buffer is too small.
size_t pixi_hexEncode (const void* input, size_t inputSize, char* output, size_t outputSize, char prefix, const char* printable);

///	Percent encode @c input to @c output.  Basically, subsitute non-alphanumeric
///	characters with %XX hex code.
///	@param input input string
///	@param inputSize length of input string
///	@param output buffer for output string
///	@param capacity of output buffer
///	@return number of input characters processed. If less than @c inputSize, output buffer is too small.
static inline size_t pixi_percentEncode (const void* input, size_t inputSize, char* output, size_t outputSize)
{
	return pixi_hexEncode (input, inputSize, output, outputSize, '%', NULL);
}

///@} defgroup

LIBPIXI_END_DECLS

#endif // !defined libpixi_util_string_h__included
