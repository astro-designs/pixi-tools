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
#include <stdlib.h>


int pixi_mpuReadRegisters16 (int fd, uint address1, int16* values, uint count)
{
	APP_PRECONDITION(fd >= 0);
	APP_PRECONDITION(count > 0);
	APP_PRECONDITION_NOT_NULL(values);

	byte request = address1;
	ssize_t written = pixi_write (fd, &request, 1);
	if (written < 0)
	{
		APP_ERROR(-written, "Failed to write MPU register request");
		return written;
	}
	else if (written == 0)
	{
		APP_LOG_ERROR("Short write of MPU register request");
		return -EIO;
	}
	byte buffer[2 * count];
	written = pixi_read (fd, &buffer, count * 2);
	if (written < 0)
	{
		APP_ERROR(-written, "Failed to read MPU register response");
		return written;
	}
	else if ((size_t) written < 2 * count)
	{
		APP_LOG_ERROR("Short read from MPU register response");
		return -EIO;
	}
	for (uint i = 0; i < count; i++)
		values[i] = int16FromBE (&buffer[i * 2]);

	return 0;
}

int pixi_mpuReadRegister16 (int fd, uint address1)
{
	int16 value = 0;
	int result = pixi_mpuReadRegisters16 (fd, address1, &value, 1);
	if (result < 0)
		return result;
	return (uint16) value;
}

int pixi_mpuWriteRegister (int fd, uint address, uint value)
{
	byte buf[] = {address, value};
	ssize_t count = pixi_write (fd, buf, sizeof (buf));
	if (count < 0)
	{
		APP_ERROR(-count, "Failed to write MPU register");
		return count;
	}
	else if (count == 0)
	{
		APP_LOG_ERROR("Short write of MPU register");
		return -EIO;
	}
	return 0;
}
