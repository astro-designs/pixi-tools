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

#ifndef libpixi_pixi_mpu_h__included
#define libpixi_pixi_mpu_h__included


#include <libpixi/common.h>
#include <limits.h>

LIBPIXI_BEGIN_DECLS

///@defgroup PiXiMPU PiXi MPU-9510A interface
///@{

enum
{
	MpuChannel    = 1,
	MpuAddress    = 0x68,
};

enum MpuRegister
{
	MpuAccelXHigh          = 0x3B,
	MpuAccelZLow           = 0x40,

	MpuTemperatureHigh     = 0x41,
	MpuTemperatureLow      = 0x42,

	MpuGyroXHigh           = 0x43,
	MpuGyroZLow            = 0x48,

	MpuPowerManagement1    = 0x6b,
};

///	Open the Pi i2c channel to the PiXi MPU. When finished,
///	call pixi_closePixi().
///	@return 0 on success, negative error code on error
int pixi_mpuOpen (void);

///	Close the Pi i2c channel to the PiXi MPU.
///	@return 0 on success, negative error code on error
int pixi_mpuClose (void);

///	Read a 16 bit unsigned value by reading from registers
///	@a address1 and @a address + 1.
///	Return non-negative value on success, negative error code on error
int pixi_mpuReadRegister16 (uint address1);

///	Read values from a sequence of 16 bit register pairs
///	Return non-negative value on success, negative error code on error
int pixi_mpuReadRegisters16 (uint address1, int16* values, uint count);

///	Write an 8 bit value to register @a address
///	Return 0 success, negative error code on error
int pixi_mpuWriteRegister (uint address, uint value);

static inline double mpuTemperatureToDegrees (int16 rawValue) {
	return (rawValue / 340.0) + 35.0;
}

///@} defgroup

LIBPIXI_END_DECLS


#endif // !defined libpixi_pixi_mpu_h__included
