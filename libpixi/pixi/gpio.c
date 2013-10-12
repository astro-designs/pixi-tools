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

#include <libpixi/pixi/gpio.h>
#include <libpixi/pixi/spi.h>
#include <libpixi/pixi/registers.h>
#include <libpixi/util/log.h>

// TODO: named constants for addresses
// TODO: check with Mark that these are really doing the right thing

static int setGpioMode (PixiGpioMode mode, int addr1, int addr2, int addr3)
{
	SpiDevice spi = SpiDeviceInit;
	int result = pixi_pixiSpiOpen (&spi);
	if (result < 0)
		return result;

	uint16_t value = mode;
	pixi_pixiSpiWriteValue16 (&spi, addr1, value);
	result = pixi_pixiSpiWriteValue16 (&spi, addr2, value);
	if (addr3 >= 0)
		result = pixi_pixiSpiWriteValue16 (&spi, addr3, value);
	pixi_spiClose (&spi);
	if (result < 0)
		LIBPIXI_ERROR(-result, "spiPixiWriteValue16");
	return 0;
}

int pixi_pixiGpioSetMode (uint gpio, PixiGpioMode mode)
{
	LIBPIXI_PRECONDITION(gpio >= 1 && gpio <= 3);

	switch (gpio)
	{
	case 1: return setGpioMode (mode, Pixi_GPIO1_00_07_mode, Pixi_GPIO1_08_15_mode, Pixi_GPIO1_16_23_mode);
	case 2: return setGpioMode (mode, Pixi_GPIO2_00_07_mode, Pixi_GPIO2_08_15_mode, -1);
	case 3: return setGpioMode (mode, Pixi_GPIO3_00_07_mode, Pixi_GPIO3_08_15_mode, -1);
	}
	return -EINVAL; // unreachable
}
