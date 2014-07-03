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

#ifndef pio_spi_h__included
#define pio_spi_h__included


#include <libpixi/pi/spi.h>

LIBPIXI_BEGIN_DECLS

///@defgroup PiXiSpi PiXi SPI interface
///@{

enum
{
	PixiSpiChannel    = 0, ///< Default value. Can also be configured to 1.
	PixiAdcSpiChannel = 1, ///< Default value. Can also be configured to 0.

	PixiSpiSpeed      = 8000000,
	PixiAdcSpiSpeed   = 1000000,

	PixiSpiEnableWrite8  = 0x40,
	PixiSpiEnableWrite16 = 0x50,
	PixiSpiEnableWrite32 = 0x60,
	PixiSpiEnableRead16  = 0x80
};

///	Open the Pi SPI channel to the pixi. When finished,
///	call pixi_closePixi().
///	@return 0 on success, or -errno on error
int pixi_openPixi (void);

///	Close the Pi SPI channel to the pixi.
///	@return 0 on success, or -errno on error
int pixi_closePixi (void);

///	Read a @c value from a PiXi register.
///	@return the register value on success, or -errno on error
int pixi_registerRead (uint address);

///	Write a @c value to a PiXi register.
///	@return the read back value on success, or -errno on error
int pixi_registerWrite (uint address, ushort value);

///	Perform a read/write to set the part of a register specified by @c mask to @c value.
///	@return the previous register value, or -errno on error
int pixi_registerWriteMasked (uint address, ushort value, ushort mask);

typedef struct RegisterOp
{
	uint8   address;    ///< PiXi register address
	uint8   function;   ///< e.g. PixiSpiEnableRead16 / PixiSpiEnableWrite16
	uint8   _valueHi;   ///< internal
	uint8   _valueLo;   ///< internal
	uint8   _reserved1; ///< internal
	uint8   _reserved2; ///< internal
	uint    value;      ///< read/write value
	ulong   userData;   ///< Ignored: available for client code
} RegisterOp;

///	Perform multiple register read/write operations in a single kernel call
///	@return 0 on success, or -errno on error
int pixi_multiRegisterOp (RegisterOp* operations, uint opCount);

///@} defgroup

LIBPIXI_END_DECLS

#endif // !defined pio_spi_h__included
