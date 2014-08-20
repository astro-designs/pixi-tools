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

#ifndef libpixi_private_h__included
#define libpixi_private_h__included


#include <libpixi/common.h>
#include <libpixi/version.h>
#include <libpixi/pixi/gpio.h>

void pixi_logInit (void);

///	Initialise GPIO library. Finds the board revision
///	and sets up the pin map.
int pixi_piGpioInit (void);

// TODO: remove
///	Set the mode of the pixi gpio 1,2 or 3.
///	@param gpio GPIO device [1,3]
///	@param mode GPIO mode
///	@return 0 on success, -errno on error
int pixi_gpioSetMode (uint gpio, PixiGpioMode mode) LIBPIXI_DEPRECATED;

#endif // !defined libpixi_private_h__included
