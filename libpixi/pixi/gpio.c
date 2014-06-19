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
	uint16_t value = mode;
	pixi_registerWrite (addr1, value);
	int result = pixi_registerWrite (addr2, value);
	if (addr3 >= 0)
		result = pixi_registerWrite (addr3, value);
	if (result < 0)
		LIBPIXI_ERROR(-result, "pixi_registerWrite");
	return result;
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

int pixi_pixiGpioSetPinMode (uint gpioController, uint pin, uint mode)
{
	LIBPIXI_PRECONDITION(gpioController >= 1 && gpioController <= 3);
	LIBPIXI_PRECONDITION(mode < 4);
	uint limit = 16;
	uint address;
	switch (gpioController)
	{
	case 1: address = Pixi_GPIO1_00_07_mode; limit = 24; break;
	case 2: address = Pixi_GPIO2_00_07_mode; break;
	case 3: address = Pixi_GPIO3_00_07_mode; break;
	default:
		LIBPIXI_PRECONDITION_FAILURE("gpioController value not recognised");
		return -EINVAL;
	}
	LIBPIXI_PRECONDITION(pin < limit);

	if (pin > 8 ) address++;
	if (pin > 16) address++;
	uint shift = 2 * (pin & 0x7);
	uint regValue = mode << shift;
	uint regMask  = 1 << shift;
	LIBPIXI_LOG_DEBUG("Setting GPIO controller=%u pin=%u mode=%u [address=0x%x regValue=0x%x]",
		gpioController, pin, mode, address, regValue);

	int result = pixi_registerWriteMasked (address, regValue, regMask);
	if (result < 0)
		LIBPIXI_ERROR(-result, "pixi_pixiWriteValueMasked");
	return result;
}

int pixi_pixiGpioWritePin (uint gpioController, uint pin, uint value)
{
	LIBPIXI_PRECONDITION(gpioController >= 1 && gpioController <= 3);
	LIBPIXI_PRECONDITION(value <= 1);
	uint limit = 16;
	uint address;
	switch (gpioController)
	{
	case 1: address = Pixi_GPIO1_00_07_IO; limit = 24; break;
	case 2: address = Pixi_GPIO2_00_07_IO; break;
	case 3: address = Pixi_GPIO3_00_07_IO; break;
	default:
		LIBPIXI_PRECONDITION_FAILURE("gpioController value not recognised");
		return -EINVAL;
	}
	LIBPIXI_PRECONDITION(pin < limit);

	if (pin > 8 ) address++;
	if (pin > 16) address++;
	uint shift = (pin & 0x7);
	uint regValue = value << shift;
	uint regMask  = 1 << shift;
	LIBPIXI_LOG_TRACE("Setting GPIO controller=%u pin=%u value=%u [address=0x%x regValue=0x%x]",
		gpioController, pin, value, address, regValue);

	// FIXME: writeMasked does not really make sense if any pins are in input mode.
	// Should instead store all register states internally.
	int result = pixi_registerWriteMasked (address, regValue, regMask);
	if (result < 0)
		LIBPIXI_ERROR(-result, "pixi_pixiWriteValueMasked");
	return result;
}

