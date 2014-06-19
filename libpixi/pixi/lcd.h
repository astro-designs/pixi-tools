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

#ifndef libpixi_pixi_lcd_h__included
#define libpixi_pixi_lcd_h__included


#include <libpixi/pi/spi.h>

LIBPIXI_BEGIN_DECLS

///@defgroup PixiLcd PiXi-200 LCD panel interface
///@{

///	Enable the LCD device on the previously opened PiXi device
///	Return 0 on success, -errno on error
int pixi_lcdEnable (void);

///	Open the PiXi (using pixi_openPixi and enable the LCD
///	(using pixi_lcdEnable).
///	Return 0 on success, -errno on error
int pixi_lcdOpen (void);

///	Initialise the LCD panel
int pixi_lcdInit (void);
int pixi_lcdInit1 (void);

///	Set the brightness of the lcd. Valid values are in range [0,3]
int pixi_lcdSetBrightness (uint value);

///	Clear the text
int pixi_lcdClear (void);

///	Move cursor to @c x, @c y
int pixi_lcdSetCursorPos (uint x, uint y);

///	Append @c str to the panel text
int pixi_lcdWriteStr (const char* str);

///@} defgroup

LIBPIXI_END_DECLS

#endif // !defined libpixi_pixi_lcd_h__included
