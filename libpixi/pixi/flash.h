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

#ifndef libpixi_pixi_flash_h__included
#define libpixi_pixi_flash_h__included


#include <libpixi/pi/spi.h>
#include <limits.h>

LIBPIXI_BEGIN_DECLS

///@defgroup PiXiFlash PiXi flash / M25P40 interface
///@{

enum
{
	FlashCapacity   =     524288, // 4 megabit
	FlashPageSize   =        256, // bytes
	FlashId         =   0x202013,

	FlashSectorSize     = 256 * FlashPageSize, // bytes
	FlashSectorBaseMask = ~(FlashSectorSize - 1) // bytes
};

enum StatusBits
{
	WriteInProgress            = 1 << 0,
	WriteEnableLatch           = 1 << 1,
	BlockProtect0              = 1 << 2,
	BlockProtect1              = 1 << 3,
	BlockProtect2              = 1 << 4,
	// 0 = 1 << 5,
	// 0 = 1 << 6,
	StatusRegisterWriteProtect = 1 << 7
};

int pixi_flashOpen (SpiDevice* device);
int pixi_flashRdpReadSig (SpiDevice* device);
int pixi_flashReadId (SpiDevice* device);
int pixi_flashReadStatus (SpiDevice* device);
int pixi_flashReadMemory (SpiDevice* device, uint address, void* buffer, uint length);
///	Erase the sectors corresponding to the region defined by @a address, @a length.
int pixi_flashEraseSectors (SpiDevice* device, uint address, uint length);
int pixi_flashWriteMemory (SpiDevice* device, uint address, const void* buffer, uint length);

int pixi_flashBulkErase (SpiDevice* device);

///@} defgroup

LIBPIXI_END_DECLS


#endif // !defined libpixi_pixi_flash_h__included
