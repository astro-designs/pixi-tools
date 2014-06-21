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

//	A PiXi simulator
//	Overrides some functions in libpixi - it's intended to be built as a
//	shared library and loaded using LD_PRELOAD.

#include <libpixi/libpixi.h>
#include <libpixi/pixi/spi.h>
#include <libpixi/util/file.h>
#include <libpixi/util/log.h>
#include <fcntl.h>
#include <string.h>
#include <unistd.h>

static uint16 registers[256];
static uint16 version[3] = {0x1213, 0x0007, 0x1031};

int pixi_getPiBoardVersion (void)
{
	return 2;
}

int pixi_spiOpen (uint channel, uint speed, SpiDevice* device)
{
	LIBPIXI_PRECONDITION(channel < 2);
	LIBPIXI_PRECONDITION_NOT_NULL(device);

	*device = SpiDeviceInit;
	const char* name = "/dev/null";
	int fd = pixi_open (name, O_RDWR, 0);
	if (fd < 0)
		return fd;

	LIBPIXI_LOG_DEBUG("Opened SPI name=%s fd=%d", name, fd);
	device->fd = fd;
	device->speed = speed;
	device->delay = 0;
	device->bitsPerWord = 8;

	if (registers[0] == 0)
		memcpy (registers, version, sizeof (version));

	return 0;
}

int pixi_spiReadWrite (SpiDevice* device, const void* outputBuffer, void* inputBuffer, size_t bufferSize)
{
	LIBPIXI_PRECONDITION_NOT_NULL(device);
	LIBPIXI_PRECONDITION(device->fd >= 0);
	LIBPIXI_PRECONDITION_NOT_NULL(outputBuffer);
	LIBPIXI_PRECONDITION_NOT_NULL(inputBuffer);
	LIBPIXI_PRECONDITION_NOT_NULL(bufferSize >= 3);

	const uint8* command = (uint8*) outputBuffer;
	uint8 address  = command[0];
	uint8 function = command[1];
	LIBPIXI_LOG_TRACE("simulated pixi_spiReadWrite address=0x%02x function=0x%02x, bufferSize=%zu", address, function, bufferSize);
	if (function == PixiSpiEnableWrite16 && bufferSize >= 4)
	{
		uint16 value = ((uint16) command[2]) << 8 | (command[3] << 8);
		LIBPIXI_LOG_TRACE("simulated write address=0x%02x value=0x%04x", address, value);
		registers[address] = value;
	}
	else if (function == PixiSpiEnableRead16 && bufferSize >= 4)
	{
		uint8 output[4] = {
			address,
			function,
			registers[address] >> 8,
			registers[address]
		};
		LIBPIXI_LOG_TRACE("simulated read address=0x%02x value=0x%04x", address, registers[address]);
		memcpy (inputBuffer, output, sizeof (output));
	}
	return 0;
}
