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

int pixi_lcdEnable (void)
{
	int result = pixi_pixiGpioSetMode (3, PixiGpioAllOutputVfdLcd);
	if (result < 0)
		LIBPIXI_ERROR (-result, "Could not set GPIO 3 mode to enable LCD");
	return result;
}

int pixi_lcdOpen (void)
{
	int result = pixi_openPixi();
	if (result < 0)
		return result;
	result = pixi_lcdEnable();
	if (result < 0) // very unlikely at this point
		pixi_closePixi();
	return result;
}

static inline int lcdWrite (uint value)
{
	return pixi_registerWrite (LcdAddress, value);
}

int pixi_lcdInit (void)
{
	int result = lcdWrite (LcdFunctionSet);
	if (result < 0)
		return result;
	lcdWrite (LcdBrightness + 3);
	lcdWrite (LcdClear);
	lcdWrite (LcdCursorHome);
	lcdWrite (LcdEntryModeSet);
	return lcdWrite (LcdDisplayOn);
}

int pixi_lcdInit1 (void)
{
	int result = lcdWrite (LcdFunctionSet);
	if (result < 0)
		return result;
	lcdWrite (LcdBrightness);
	lcdWrite (LcdClear);
	lcdWrite (LcdCursorHome);
	lcdWrite (LcdEntryModeSet);
	return lcdWrite (0x000F);
}

int pixi_lcdSetBrightness (uint value)
{
	lcdWrite (LcdFunctionSet);
	return lcdWrite (LcdBrightness + (value & 0x03));
}

int pixi_lcdClear (void)
{
	lcdWrite (LcdFunctionSet);
	lcdWrite (LcdClear);
	return lcdWrite (LcdCursorHome);
}


int pixi_lcdSetCursorPos (uint x, uint y)
{
	x &= 0x3F;
	y &= 1;
	LIBPIXI_LOG_DEBUG("Setting cursor position to: %d, %d", x, y);
	uint value = 0x0080 + (y << 6) + x;
	return lcdWrite (value);
}

int pixi_lcdWriteStr (const char* str)
{
	LIBPIXI_PRECONDITION_NOT_NULL(str);

	LIBPIXI_LOG_DEBUG("Writing str to LCD: [%s]", str);
	uint len = strlen (str);
	int result = 0;
	for (uint i = 0; i < len; i++)
	{
		uint value = 0x00000200 + (uint8_t) str[i]; // Set RS to '1' for display data & combine upper / lower bytes
      	result = lcdWrite (value);
	}
	return result;
}
