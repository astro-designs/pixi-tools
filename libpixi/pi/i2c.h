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

#ifndef libpixi_pi_i2c_h__included
#define libpixi_pi_i2c_h__included


#include <libpixi/common.h>
#include <stddef.h>

LIBPIXI_BEGIN_DECLS

///@defgroup PiI2C Raspberry Pi i2c interface
///@{

///	Open the i2c device on @c channel, at @c address
///	@return file descriptor on success, or negative error code
int pixi_i2cOpen (uint channel, uint address);

typedef struct I2cDevice
{
	int      fd;          ///< file descriptor
	uint     address;
	intptr   _reserved[2];
} I2cDevice;

#define I2C_DEVICE_INIT {-1, 0, {0,0}}
static const I2cDevice I2cDeviceInit = I2C_DEVICE_INIT;

///	Open the i2c device on @c channel, at @c address.
///	On success, the fields of @c device are filled out, and
///	you will need to subsequently call @ref pixi_i2cClose().
///	@return 0 on success, or -errno on error
int pixi_i2cOpen2 (uint channel, uint address, I2cDevice* device);

int pixi_i2cClose (I2cDevice* device);

///	Perform a read/write on a i2c device opened via pixi_spiOpen()
///	@return 0 on success, or -errno on error
int pixi_i2cReadWrite (I2cDevice* device, const void* txBuffer, size_t txSize, void* rxBuffer, size_t rxSize);

///@} defgroup

LIBPIXI_END_DECLS

#endif // !defined libpixi_pi_i2c_h__included
