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

#include <libpixi/pixi/dac.h>
#include <libpixi/pi/i2c.h>
#include <libpixi/util/file.h>
#include <libpixi/util/log.h>
#include <stdlib.h>

static int dacI2c = -1;

int pixi_dacOpen (void)
{
	// TODO: instead rejecting if previously open,
	// do ref-counting of open count?
	LIBPIXI_PRECONDITION(dacI2c < 0);
	int result = pixi_i2cOpen (PixiDacChannel, PixiDacAddress);
	if (result < 0)
		LIBPIXI_ERROR(-result, "Cannot open i2c channel to PiXi DAC");
	dacI2c = result;
	return result;
}

int pixi_dacClose (void)
{
	LIBPIXI_PRECONDITION(dacI2c >= 0);
	return pixi_close (dacI2c);
}

int pixi_dacWriteValue (uint channel, uint value)
{
	LIBPIXI_PRECONDITION(dacI2c >= 0);
	LIBPIXI_PRECONDITION(channel < PixiDacChannels);
	LIBPIXI_PRECONDITION(value < 4096);

	byte buf[] = {
		PixiDacMultiWrite + (channel << 1),
		(value >> 8) & 0x0F,
		value
	};
	LIBPIXI_LOG_TRACE("Setting DAC channel %u=%u", channel, value);
	LIBPIXI_LOG_DEBUG("Writing to DAC i2c: %02x %02x %02x", buf[0], buf[1], buf[2]);
	ssize_t count = pixi_write (dacI2c, buf, sizeof (buf));
	if (count < 0)
	{
		APP_ERROR(-count, "Failed to write DAC value");
		return count;
	}
	else if (count == 0)
	{
		APP_LOG_ERROR("Short write of DAC value");
		return -EIO;
	}
	return 0;
}
