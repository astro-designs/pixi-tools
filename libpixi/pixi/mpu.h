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
#include <stddef.h>

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
	MpuGyroConfig          = 0x1B,
	MpuAccelConfig         = 0x1C,

	/// Big-endian signed 16 bit
	MpuAccelXHigh          = 0x3B,
	MpuAccelXLow,
	MpuAccelYHigh,
	MpuAccelYLow,
	MpuAccelZHigh,
	MpuAccelZLow,

	MpuTemperatureHigh     = 0x41,
	MpuTemperatureLow,

	MpuGyroXHigh           = 0x43,
	MpuGyroXLow,
	MpuGyroYHigh,
	MpuGyroYLow,
	MpuGyroZHigh,
	MpuGyroZLow,

	MpuPowerManagement1    = 0x6b,
};

typedef struct MpuAxes
{
	int16   x;
	int16   y;
	int16   z;
} MpuAxes;

///	Directly maps to MPU gyroscope/temperature/accelerometer registers
typedef struct MpuMotion
{
	struct MpuAxes   accel;
	int16            temp;
	struct MpuAxes   gyro;
} MpuMotion;

///	Open the Pi i2c channel to the PiXi MPU. When finished,
///	call pixi_closePixi().
///	@return 0 on success, negative error code on error
int pixi_mpuOpen (void);

///	Close the Pi i2c channel to the PiXi MPU.
///	@return 0 on success, negative error code on error
int pixi_mpuClose (void);

///	Read a 8 bit unsigned value by reading from register
///	@a address1
///	Return non-negative value on success, or negative error code
int pixi_mpuReadRegister (uint address);

///	Read a 16 bit little-endian unsigned value by reading from registers
///	@a address1 and @a address + 1.
///	Return non-negative value on success, negative error code on error
int pixi_mpuReadRegister16 (uint address1);

///	Read raw data from MPU registers starting at @a address1.
///	Return 0 on success, negative error code on error
int pixi_mpuReadRegisters (uint address1, void* buffer, size_t size);

///	Read values from a sequence of 16 bit big-endian register pairs
///	(as used by gyroscope, temperatue and accelerometer components).
///	Return 0 on success, negative error code on error
int pixi_mpuReadRegisters16 (uint address1, int16* values, uint count);

///	Write an 8 bit value to register @a address
///	@return 0 on success, negative error code on error
int pixi_mpuWriteRegister (uint address, uint value);

///	Get the accelerometer scale.
///	@return 2,4,8,16 (g) on success, or negative error code
int pixi_mpuGetAccelScale (void);

///	Set the gyroscope scale
///	@param scale 250,500,1000 or 2000 (dps)
///	@return 0 on success, or negative error code
int pixi_mpuSetGyroScale (uint scale);

///	Read current accelerometer values.
///	@return 0 on success, or negative error code
int pixi_mpuReadAccel (MpuAxes* axes);

///	Set the accelerometer scale
///	@param scale 2,4,8 or 16 (g)
///	@return 0 on success, or negative error code
int pixi_mpuSetAccelScale (uint scale);

///	Get the gyroscope scale.
///	@return 250,500,1000 or 2000 (dps) on success, or negative error code
int pixi_mpuGetGyroScale (void);

///	Read current gyroscope values.
///	@return 0 on success, or negative error code
int pixi_mpuReadGyro (MpuAxes* axes);

///	Read current gyroscope, accelerometer and temperature values.
///	@return 0 on success, or negative error code
int pixi_mpuReadMotion (MpuMotion* motion);

static inline double mpuTemperatureToDegrees (int16 rawValue) {
	return (rawValue / 340.0) + 35.0;
}

///@} defgroup

LIBPIXI_END_DECLS


#endif // !defined libpixi_pixi_mpu_h__included
