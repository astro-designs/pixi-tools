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
static I2cDevice mpuMagI2c = I2C_DEVICE_INIT;

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
	if (mpuMagI2c.fd >= 0)
		pixi_mpuMagClose();
	return pixi_i2cClose (&mpuI2c);
}

int pixi_mpuMagOpen (void)
{
	// TODO: instead rejecting if previously open,
	// do ref-counting of open count?
	LIBPIXI_PRECONDITION_MSG(mpuMagI2c.fd < 0, "MPU must be open");
	LIBPIXI_PRECONDITION_MSG(mpuI2c.fd >= 0, "MPU MAG must not be open");
	LIBPIXI_LOG_DEBUG("Enabling MPU i2c bypass for magnetometer");
	int result = pixi_mpuWriteRegisterMasked (MpuIntBypassConfig, MpuIntCfgBypassEn, MpuIntCfgBypassEn);
	if (result < 0)
		LIBPIXI_ERROR(-result, "Failed to enable MPU bypass mode (for magnetometer)");
	else
		result = pixi_i2cOpen2 (MpuChannel, MpuMagAddress, &mpuMagI2c);
	return result;
}

int pixi_mpuMagClose (void)
{
	LIBPIXI_PRECONDITION_MSG(mpuMagI2c.fd >= 0, "MPU MAG must be open");
	LIBPIXI_LOG_DEBUG("Disabling MPU i2c bypass for magnetometer");
	int result = pixi_mpuWriteRegisterMasked (MpuIntBypassConfig, 0, MpuIntCfgBypassEn);
	if (result < 0)
		LIBPIXI_ERROR(-result, "Failed to disable MPU bypass mode (for magnetometer)");

	return pixi_i2cClose (&mpuMagI2c);
}

static int readRegisters (I2cDevice* device, uint address1, void* buffer, size_t size)
{
	LIBPIXI_PRECONDITION_NOT_NULL(device);
	LIBPIXI_PRECONDITION(device->fd >= 0);
	LIBPIXI_PRECONDITION(size > 0);
	LIBPIXI_PRECONDITION(size <= 256);
	LIBPIXI_PRECONDITION_NOT_NULL(buffer);

	byte request = address1;
	int result = pixi_i2cWriteRead (device,
		&request, sizeof (request),
		buffer, size
		);
	if (result < 0)
	{
		LIBPIXI_ERROR(-result, "pixi_i2cWriteRead failed for MPU");
		return result;
	}

	return 0;
}

int pixi_mpuReadRegisters (uint address1, void* buffer, size_t size)
{
	return readRegisters (&mpuI2c, address1, buffer, size);
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

int pixi_mpuReadRegister (uint address)
{
	uint8 value = 0;
	int result = pixi_mpuReadRegisters (address, &value, 1);
	if (result < 0)
		return result;
	return value;
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

int pixi_mpuWriteRegisterMasked (uint address, uint value, uint mask)
{
	int previous = pixi_mpuReadRegister (address);
	if (previous < 0)
		return previous;
	ushort masked = (value & mask) | (previous & ~mask);
	int result = pixi_mpuWriteRegister (address, masked);
	if (result < 0)
		return result;
	return previous;
}

int pixi_mpuMagReadRegisters (uint address1, void* buffer, size_t size)
{
	return readRegisters (&mpuMagI2c, address1, buffer, size);
}

int pixi_mpuMagReadRegister (uint address)
{
	uint8 value = 0;
	int result = pixi_mpuMagReadRegisters (address, &value, sizeof (value));
	if (result < 0)
		return result;
	return value;
}

int pixi_mpuMagReadRegister16 (uint address)
{
	uint16 value = 0;
	int result = pixi_mpuMagReadRegisters (address, &value, sizeof (value));
	if (result < 0)
		return result;
	return value;
}

int pixi_mpuGetAccelScale()
{
	int result = pixi_mpuReadRegister (MpuAccelConfig);
	if (result < 0)
		return result;
	uint select = (result >> 3) & 0x3;
	return 2 << select;
}

int pixi_mpuSetAccelScale (uint scale)
{
	uint select;
	switch (scale)
	{
	case  2: select = 0; break;
	case  4: select = 1; break;
	case  8: select = 2; break;
	case 16: select = 3; break;
	default:
		LIBPIXI_PRECONDITION_FAILURE("accelerometer scale should be 2,4,8 or 16");
		return -EINVAL;
	}
	return pixi_mpuWriteRegister (MpuAccelConfig, select << 3);
}

int pixi_mpuReadAccel (MpuAxes* axes)
{
	LIBPIXI_PRECONDITION_NOT_NULL(axes);
	return pixi_mpuReadRegisters16 (MpuAccelXHigh, &axes->x, sizeof (*axes) / sizeof (int16));
}

int pixi_mpuGetGyroScale()
{
	int result = pixi_mpuReadRegister (MpuGyroConfig);
	if (result < 0)
		return result;
	uint select = (result >> 3) & 0x3;
	return 250 << select;
}

int pixi_mpuSetGyroScale (uint scale)
{
	uint select;
	switch (scale)
	{
	case  250: select = 0; break;
	case  500: select = 1; break;
	case 1000: select = 2; break;
	case 2000: select = 3; break;
	default:
		LIBPIXI_PRECONDITION_FAILURE("gyroscope scale should be 250,500,1000 or 2000");
		return -EINVAL;
	}
	return pixi_mpuWriteRegister (MpuGyroConfig, select << 3);
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

int pixi_mpuReadMagAdjust (MpuAxes* axes)
{
	LIBPIXI_PRECONDITION_MSG(mpuMagI2c.fd >= 0, "MPU MAG must be open");
	LIBPIXI_PRECONDITION_NOT_NULL(axes);

	uint8 values[3];
	int result = pixi_mpuMagReadRegisters (MpuMagXAdjust, values, sizeof (values));
	if (result < 0)
		return result;

	axes->x = values[0];
	axes->y = values[1];
	axes->z = values[2];
	return 0;
}

int pixi_mpuReadMag (MpuAxes* axes)
{
	LIBPIXI_PRECONDITION_MSG(mpuMagI2c.fd >= 0, "MPU MAG must be open");
	LIBPIXI_PRECONDITION_NOT_NULL(axes);

	// write data read request to control
	uint8 control[] = {MpuMagControl, MpuMagSingleMeasurementMode};
	int result = pixi_i2cWriteRead (&mpuMagI2c, control, sizeof (control), NULL, 0);
	if (result < 0)
		return result;

	// Read status and values at the same time.
	struct {
		uint8 info; // alignment
		uint8 status1;
		MpuAxes axes;
	} values;
	// Data is typically ready on seventh read
	for (int i = 0; i < 20; i++)
	{
		result = pixi_mpuMagReadRegisters (MpuMagStatus1, &values.status1, sizeof (values) - sizeof (values.info));
		if (result < 0)
			return result;
		if (values.status1 & MpuMagDataReady)
		{
			// OK, Pi is little-endian, but be explicit
			axes->x = le16toh (values.axes.x);
			axes->y = le16toh (values.axes.y);
			axes->z = le16toh (values.axes.z);
			return 0;
		}
	}
	LIBPIXI_LOG_ERROR("Timed out trying to read MPU MAG values");
	return -ETIMEDOUT;
}
