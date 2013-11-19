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

#include <libpixi/pixi/spi.h>
#include <libpixi/util/log.h>

int pixi_pixiSpiOpen (SpiDevice* device) {
	int result = pixi_spiOpen (PixiSpiChannel, PixiSpiSpeed, device);
	if (result < 0)
		LIBPIXI_ERROR(-result, "Cannot open SPI channel to pixi");
	return result;
}

static int readWriteValue16 (SpiDevice* device, uint function, uint address, uint16 value)
{
	uint8_t buffer[4] = {
		address,
		function,
		(value & 0xFF00) >> 8,
		(value & 0x00FF)
	};
	int result = pixi_spiReadWrite(device, buffer, buffer, sizeof (buffer));
	if (result < 0)
		return result;
	return (buffer[2] << 8) | buffer[3];
}

int pixi_pixiSpiWriteValue16 (SpiDevice* device, uint address, uint16_t value)
{
	return pixi_registerWrite (device, address, value);
}

int pixi_pixiSpiReadValue16 (SpiDevice* device, uint address)
{
	return pixi_registerRead (device, address);
}

int pixi_registerRead (SpiDevice* device, uint address)
{
	int result = readWriteValue16 (device, PixiSpiEnableRead16, address, 0);
	LIBPIXI_LOG_DEBUG("pixi_pixiWriteRegister address=0x%02x result=%d", address, result);
	return result;
}

int pixi_registerWrite (SpiDevice* device, uint address, ushort value)
{
	int result = readWriteValue16 (device, PixiSpiEnableWrite16, address, value);
	LIBPIXI_LOG_DEBUG("pixi_pixiWriteRegister address=0x%02x value=0x%04x result=%d", address, value, result);
	return result;
}

int pixi_registerWriteMasked (SpiDevice* device, uint address, ushort value, ushort mask)
{
	int previous = pixi_registerRead (device, address);
	if (previous < 0)
		return previous;
	ushort masked = (value & mask) | (previous & ~mask);
	int result = pixi_registerWrite (device, address, masked);
	if (result < 0)
		return result;
	return previous;
}
