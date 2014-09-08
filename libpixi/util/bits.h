/*
    pixi-tools: a set of software to interface with the Raspberry Pi
    and PiXi-200 hardware
    Copyright (C) 2014 Simon Cantrill

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

#ifndef libpixi_util_bits_h__included
#define libpixi_util_bits_h__included


#include <libpixi/common.h>

LIBPIXI_BEGIN_DECLS

///@defgroup util_bits libpixi bit mangling utilities
///@{

///	Make a signed 16 bit integer from high / low bytes
static inline int16 makeInt16 (uint8 hi, uint8 lo) {
	return (((uint) hi) << 8) | lo;
}

///	Make a signed 16 bit integer from big-endian byte array
static inline int16 int16FromBE (const uint8* hi) {
	return makeInt16 (hi[0], hi[1]);
}

///	Make a signed 16 bit integer from little-endian byte array
static inline int16 int16FromLE (const uint8* lo) {
	return makeInt16 (lo[1], lo[0]);
}

///	Make unsigned 12 bit integer from signed high nibble / low byte
static inline uint16 makeUint12 (uint8 hi, uint8 lo)
{
	return (((uint) hi & 0x0F) << 8) | lo;
}

///	Make signed 12 bit integer from signed high nibble / low byte
static inline int16 makeInt12 (uint8 hi, uint8 lo)
{
	const int signExt[2] = {
		0,
		-1 & ~0xFFF
	};
	return signExt[1 && (hi & 0x08)]
	            | ((hi & 0x0F) << 8)
	            | lo;
}

///@} defgroup

LIBPIXI_END_DECLS

#endif // !defined libpixi_util_bits_h__included
