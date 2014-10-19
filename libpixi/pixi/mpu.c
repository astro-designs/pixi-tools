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

#include <libpixi/pixi/mpu.h>
#include <libpixi/pi/i2c.h>
#include <libpixi/util/bits.h>
#include <libpixi/util/file.h>
#include <libpixi/util/log.h>
#include <endian.h>
#include <stdlib.h>


static I2cDevice mpuI2c = I2C_DEVICE_INIT;

int pixi_mpuOpen (void)
{
	// TODO: instead rejecting if previously open,
	// do ref-counting of open count?
	LIBPIXI_PRECONDITION(mpuI2c.fd < 0);
	int result = pixi_i2cOpen2 (MpuChannel, MpuAddress, &mpuI2c);
	if (result < 0)
		LIBPIXI_ERROR(-result, "Cannot open i2c channel to PiXi MPU");
	return result;
}

int pixi_mpuClose (void)
{
	LIBPIXI_PRECONDITION(mpuI2c.fd >= 0);
	return pixi_i2cClose (&mpuI2c);
}

int pixi_mpuReadRegisters (uint address1, void* buffer, size_t size)
{
	LIBPIXI_PRECONDITION(mpuI2c.fd >= 0);
	LIBPIXI_PRECONDITION(size > 0);
	LIBPIXI_PRECONDITION(size <= 256);
	LIBPIXI_PRECONDITION_NOT_NULL(buffer);

	byte request = address1;
	int result = pixi_i2cReadWrite (&mpuI2c,
		&request, sizeof (request),
		buffer, size
		);
	if (result < 0)
	{
		LIBPIXI_ERROR(-result, "pixi_i2cReadWrite failed for MPU");
		return result;
	}

	return 0;

}

int pixi_mpuReadRegisters16 (uint address1, int16* values, uint count)
{
	LIBPIXI_PRECONDITION(mpuI2c.fd >= 0);
	LIBPIXI_PRECONDITION(count > 0);
	LIBPIXI_PRECONDITION_NOT_NULL(values);

	int result = pixi_mpuReadRegisters (address1, values, count * sizeof (*values));
	if (result < 0)
		return result;

	for (uint i = 0; i < count; i++)
		values[i] = be16toh (values[i]);

	return 0;
}

int pixi_mpuReadRegister16 (uint address1)
{
	int16 value = 0;
	int result = pixi_mpuReadRegisters16 (address1, &value, 1);
	if (result < 0)
		return result;
	return (uint16) value;
}

int pixi_mpuWriteRegister (uint address, uint value)
{
	LIBPIXI_PRECONDITION(mpuI2c.fd >= 0);
	LIBPIXI_PRECONDITION(address < 128);

	byte buf[] = {address, value};
	ssize_t count = pixi_write (mpuI2c.fd, buf, sizeof (buf));
	if (count < 0)
	{
		LIBPIXI_ERROR(-count, "Failed to write MPU register");
		return count;
	}
	else if (count == 0)
	{
		LIBPIXI_LOG_ERROR("Short write of MPU register");
		return -EIO;
	}
	return 0;
}

int pixi_mpuReadAccel (MpuAxes* axes)
{
	LIBPIXI_PRECONDITION_NOT_NULL(axes);
	return pixi_mpuReadRegisters16 (MpuAccelXHigh, &axes->x, sizeof (*axes) / sizeof (int16));
}

int pixi_mpuReadGyro (MpuAxes* axes)
{
	LIBPIXI_PRECONDITION_NOT_NULL(axes);
	return pixi_mpuReadRegisters16 (MpuGyroXHigh, &axes->x, sizeof (*axes) / sizeof (int16));
}

int pixi_mpuReadMotion (MpuMotion* motion)
{
	LIBPIXI_PRECONDITION_NOT_NULL(motion);
	return pixi_mpuReadRegisters16 (MpuAccelXHigh, &motion->accel.x, sizeof (*motion) / sizeof (int16));
}
