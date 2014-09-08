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

#ifndef libpixi_pixi_dac_h__included
#define libpixi_pixi_dac_h__included


#include <libpixi/common.h>
#include <limits.h>

LIBPIXI_BEGIN_DECLS

///@defgroup PiXiDAC PiXi DAC / MCP4728 interface
///@{

enum
{
	PixiDacChannel         = 1,
	PixiDacAddress         = 0x60,
	PixiDacChannels        = 4,
};

enum PixiDacCommands
{
	PixiDacMultiWrite      = 0x40, ///< + (channel < 1) + UDAC-bit, followed by 2 more bytes
};

///	Open the Pi i2c channel to the PiXi DAC. When finished,
///	call pixi_closePixi().
///	@return 0 on success, negative error code on error
int pixi_dacOpen (void);

///	Close the Pi i2c channel to the PiXi DAC.
///	@return 0 on success, negative error code on error
int pixi_dacClose (void);

///	Write a value to a DAC channel.
///	@return 0 on success, negative error code on error
int pixi_dacWriteValue (uint channel, uint value);

///@} defgroup

LIBPIXI_END_DECLS


#endif // !defined libpixi_pixi_dac_h__included
