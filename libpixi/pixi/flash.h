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


#include <libpixi/common.h>
#include <limits.h>

LIBPIXI_BEGIN_DECLS

///@defgroup PiXiFlash PiXi flash / M25P40 interface
///@{

enum FlashSpec
{
	FlashCapacity   =     524288, // 4 megabit
	FlashPageSize   =        256, // bytes
	FlashId         =   0x202013,

	FlashSectorSize     = 256 * FlashPageSize, // bytes
	FlashSectorBaseMask = ~(FlashSectorSize - 1) // bytes
};

enum FlashStatusBits
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

///	Open the Pi SPI channel to the pixi flash. When finished,
///	call pixi_closePixi().
///	@return 0 on success, or negative error code on error
int pixi_flashOpen (void);

///	Close the Pi SPI channel to the pixi flash.
///	@return 0 on success, or negative error code on error
int pixi_flashClose (void);

///	Execute the 'Release from deep power down and read
///	electronic signature' instruction.
///	@return 8 bit unsigned signature value on success, or negative error code on error.
int pixi_flashRdpReadSig (void);

///	Read the flash device id. Should equal #FlashId.
///	@return 24 bit unsigned id value on success, or negative error code on error.
int pixi_flashReadId (void);

///	Read the flash status register.
///	@see StatusBits
///	@return 8 bit unsigned bitmap, or negative error code on error.
int pixi_flashReadStatus (void);

///	Read from the flash memory.
///	@param	address	flash memory address offset
///	@param	buffer	destination buffer
///	@param	length	number of bytes to read
///	@return number of bytes read, or negative error code on error.
int pixi_flashReadMemory (uint address, void* buffer, uint length);

///	Erase the sectors corresponding to a memory region.
///	Note that this erases all the sectors (#FlashSectorSize)
///	that the memory region overlaps.
///	@param	address	flash memory address offset
///	@param	length	number of bytes to read
///	@return number of bytes erased, or negative error code on error.
int pixi_flashEraseSectors (uint address, uint length);

///	Write to flash memory. Note that the write process only changes
///	0 value bits to 1, so you may need to erase the memory first,
///	using @c pixi_flashEraseSectors (@a address, @a length);
///	@param	address	flash memory address offset
///	@param	buffer	source buffer
///	@param	length	number of bytes to write
///	@return number of bytes written, or negative error code on error.
int pixi_flashWriteMemory (uint address, const void* buffer, uint length);

///	Erase the entire flash memory.
///	@return 0 on success, or negative error code on error.
int pixi_flashBulkErase (void);

///@} defgroup

LIBPIXI_END_DECLS


#endif // !defined libpixi_pixi_flash_h__included
