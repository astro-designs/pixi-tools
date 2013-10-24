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

#include <libpixi/pixi/lcd.h>
#include <libpixi/pixi/gpio.h>
#include <libpixi/pixi/registers.h>
#include <libpixi/pixi/spi.h>
#include <libpixi/util/log.h>
#include <string.h>

enum
{
	LcdAddress = Pixi_VFDLCD_out,

	LcdClear        = 0x0001,
	LcdCursorHome   = 0x0002,
	LcdEntryModeSet = 0x0006,
	LcdDisplayOn    = 0x000C,
	LcdFunctionSet  = 0x0030,
	LcdBrightness   = 0x0200
};

static int setGpioModeForLcd (void)
{
	// TODO:
	int result = pixi_pixiGpioSetMode (3, PixiGpioAllOutputVfdLcd);
	if (result < 0)
		LIBPIXI_ERROR (-result, "Could not set GPIO 3 mode for LCD");
	return result;
}

int pixi_lcdOpen (LcdDevice* device)
{
	LIBPIXI_PRECONDITION_NOT_NULL(device);
	int result = setGpioModeForLcd();
	if (result < 0)
		return result;
	return pixi_pixiSpiOpen (&device->spi);
}

int pixi_lcdClose (LcdDevice* device)
{
	LIBPIXI_PRECONDITION_NOT_NULL(device);
	return pixi_spiClose (&device->spi);
}

static inline int lcdWrite (LcdDevice* device, uint value)
{
	LIBPIXI_PRECONDITION_NOT_NULL(device);
	return pixi_pixiSpiWriteValue16 (&device->spi, LcdAddress, value);
}

int pixi_lcdInit (LcdDevice* device)
{
	LIBPIXI_PRECONDITION_NOT_NULL(device);

	lcdWrite (device, LcdFunctionSet);
	lcdWrite (device, LcdBrightness + 3);
	lcdWrite (device, LcdClear);
	lcdWrite (device, LcdCursorHome);
	lcdWrite (device, LcdEntryModeSet);
	return lcdWrite (device, LcdDisplayOn);
}

int pixi_lcdInit1 (LcdDevice* device)
{
	LIBPIXI_PRECONDITION_NOT_NULL(device);

	lcdWrite (device, LcdFunctionSet);
	lcdWrite (device, LcdBrightness);
	lcdWrite (device, LcdClear);
	lcdWrite (device, LcdCursorHome);
	lcdWrite (device, LcdEntryModeSet);
	return lcdWrite (device, 0x000F);
}

int pixi_lcdSetBrightness (LcdDevice* device, uint value)
{
	LIBPIXI_PRECONDITION_NOT_NULL(device);

	lcdWrite (device, LcdFunctionSet);
	return lcdWrite (device, LcdBrightness + (value & 0x03));
}

int pixi_lcdClear (LcdDevice* device)
{
	LIBPIXI_PRECONDITION_NOT_NULL(device);

	lcdWrite (device, LcdFunctionSet);
	lcdWrite (device, LcdClear);
	return lcdWrite (device, LcdCursorHome);
}


int pixi_lcdSetCursorPos (LcdDevice* device, uint x, uint y)
{
	LIBPIXI_PRECONDITION_NOT_NULL(device);

	LIBPIXI_LOG_DEBUG("Setting cursor position to: %d, %d", x, y);
	uint value = 0x0080 + ((y & 0x3f) << 6) + (x & 0x3f);
	return lcdWrite (device, value);
}

int pixi_lcdWriteStr (LcdDevice* device, const char* str)
{
	LIBPIXI_PRECONDITION_NOT_NULL(device);
	LIBPIXI_PRECONDITION_NOT_NULL(str);

	LIBPIXI_LOG_DEBUG("Writing str to LCD: [%s]", str);
	uint len = strlen (str);
	int result = 0;
	for (uint i = 0; i < len; i++)
	{
		uint value = 0x00000200 + (uint8_t) str[i]; // Set RS to '1' for display data & combine upper / lower bytes
      	result = lcdWrite (device, value);
	}
	return result;
}
