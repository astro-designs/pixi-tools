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

#ifndef libpixi_pixi_gpio_h__included
#define libpixi_pixi_gpio_h__included


#include <libpixi/common.h>

LIBPIXI_BEGIN_DECLS

///@defgroup PixiGpio PiXi-200 GPIO interface
///@{

// TODO: check all this with Mark

typedef enum PixiGpioMode
{
	PixiGpioAllInput,
	PixiGpioAllOutputTiedOut,
	PixiGpioAllOutputVfdLcd,
	PixiGpio3,
} PixiGpioMode;

///	Set the mode of the pixi gpio 1,2 or 3.
///	@return 0 on success, -errno on error
int pixi_pixiGpioSetMode (uint gpio, PixiGpioMode mode);

///@} defgroup

LIBPIXI_END_DECLS

#endif // !defined libpixi_pixi_gpio_h__included
