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
#include <libpixi/pixi/adc.h>
#include <libpixi/util/log.h>
#include <stdlib.h>

///@defgroup PiXiSimple PiXi simple interface
///@{

extern SpiDevice globalPixi;
extern SpiDevice globalPixiAdc;

///	Open the global SPI channel to the PiXi.
///	@return PiXi FPGA version on success, or -errno on error
static inline int64 pixiOpen (void) {
	int result = pixi_pixiSpiOpen (&globalPixi);
	if (result < 0)
	{
		LIBPIXI_ERROR(-result, "Failed to open PiXi SPI channel");
		return result;
	}
	int64 version = pixi_pixiFpgaGetVersion (&globalPixi);
	if (version <= 0)
		LIBPIXI_LOG_ERROR("Failed to get valid PiXi FPGA version, got: %d", (int) result);
	return version;
}

///	Open the global SPI channel to the PiXi. Exit the process on error.
///	@return PiXi FPGA version on success, no return on error
static inline int64 pixiOpenOrDie (void) {
	int64 version = pixiOpen();
	if (version <= 0)
	{
		LIBPIXI_LOG_ERROR("Aborting");
		exit (255);
	}
	return version;
}

///	Close the global SPI channel to the PiXi.
static inline int pixiClose (void) {
	return pixi_spiClose (&globalPixi);
}

///	Open the global SPI channel to the PiXi ADC.
///	@return 0 on success, or -errno on error
static inline int pixiAdcOpen (void) {
	int result = pixi_pixiAdcOpen (&globalPixiAdc);
	if (result < 0)
		LIBPIXI_ERROR(-result, "Failed to open PiXi ADC SPI channel");
	return result;
}

///	Open the global SPI channel to the PiXi ADC. Exit the process on error.
///	@return PiXi FPGA version on success, no return on error
static inline int64 pixiAdcOpenOrDie (void) {
	int result = pixiAdcOpen();
	if (result < 0)
	{
		LIBPIXI_LOG_ERROR("Aborting");
		exit (254);
	}
	return result;
}

///	Close the global SPI channel to the PiXi ADC.
static inline int pixiAdcClose (void) {
	return pixi_spiClose (&globalPixiAdc);
}

///	Wrapper for @ref pixi_registerRead
static inline int registerRead (uint address) {
	return pixi_registerRead (&globalPixi, address);
}

///	Wrapper for @ref pixi_registerWrite
static inline int registerWrite (uint address, ushort value) {
	return pixi_registerWrite (&globalPixi, address, value);
}

///	Wrapper for @ref pixi_multiRegisterOp
static inline int multiRegisterOp (RegisterOp* operations, uint opCount) {
	return pixi_multiRegisterOp (&globalPixi, operations, opCount);
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

///	Wrapper for @ref pixi_pwmWritePinPercent
static inline int pwmWritePinPercent (uint pin, double dutyCycle) {
	return pixi_pwmWritePinPercent (&globalPixi, pin, dutyCycle);
}

///	Wrapper for @ref pixi_pixiAdcRead
static inline int adcRead (uint adcChannel) {
	return pixi_pixiAdcRead (&globalPixiAdc, adcChannel);
}

///@} defgroup

#endif // !defined libpixi_pixi_simple_h__included
