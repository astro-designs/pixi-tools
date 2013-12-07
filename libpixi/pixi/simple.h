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

#ifndef libpixi_pixi_simple_h__included
#define libpixi_pixi_simple_h__included

#include <libpixi/pixi/spi.h>
#include <libpixi/pixi/fpga.h>
#include <libpixi/pixi/gpio.h>
#include <libpixi/pixi/pwm.h>
#include <stdlib.h>

///@defgroup PiXiSimple PiXi simple interface
///@{

extern SpiDevice globalPixi;

///	Open the global SPI channel to the PiXi.
///	@return PiXi FPGA version on success, or -errno on error
static inline int64 pixiOpen (void) {
	int result = pixi_pixiSpiOpen (&globalPixi);
	if (result < 0)
		return result;
	return pixi_pixiFpgaGetVersion();
}

///	Open the global SPI channel to the PiXi. Exit the process on error.
///	@return PiXi FPGA version on success, no return on error
static inline int64 pixiOpenOrDie (void) {
	int64 version = pixiOpen();
	if (version <= 0)
		exit (255);
	return version;
}

///	Close the global SPI channel to the PiXi.
static inline int pixiClose (void) {
	return pixi_spiClose (&globalPixi);
}


///	Wrapper for @ref pixi_pixiGpioSetPinMode
static inline int gpioSetPinMode (uint gpioController, uint pin, uint mode) {
	return pixi_pixiGpioSetPinMode (&globalPixi, gpioController, pin, mode);
}

///	Wrapper for @ref pixi_pixiGpioWritePin
static inline int gpioWritePin (uint gpioController, uint pin, uint value) {
	return pixi_pixiGpioWritePin (&globalPixi, gpioController, pin, value);
}

///	Wrapper for @ref pixi_pwmWritePin
static inline int pwmWritePin (uint pin, uint dutyCycle) {
	return pixi_pwmWritePin (&globalPixi, pin, dutyCycle);
}

///@} defgroup

#endif // !defined libpixi_pixi_simple_h__included
