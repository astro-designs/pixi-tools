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

#ifndef libpixi_pi_gpio_h__included
#define libpixi_pi_gpio_h__included


#include <libpixi/common.h>

LIBPIXI_BEGIN_DECLS

///@defgroup PiGpio Raspberry Pi GPIO interface
///@{

static const uint GpioNumPins = 54;

typedef enum Direction
{
	DirectionIn,
	DirectionOut
} Direction;

typedef enum Edge
{
	EdgeNone,
	EdgeRising,
	EdgeFalling,
	EdgeBoth
} Edge;

typedef struct GpioState
{
	bool       exported;
	Direction  direction;
	int        value;
	Edge       edge;
	int        activeLow;
} GpioState;

enum PinValue
{
	Low  = 0,
	High = 1
};

///	Parse a string as an Direction value.
///	@return enum Direction, or <0 on error
int pixi_piGpioStrToDirection (const char* direction);

///	Convert a Direction to a string.
///	@return a direction string, or "[invalid-direction]" on error
const char* pixi_piGpioDirectionToStr (Direction direction);

///	Parse a string as an Edge value.
///	@return enum Edge, or <0 on error
int pixi_piGpioStrToEdge (const char* edge);

///	Convert an Edge to a string.
///	@return an Edge string, or "[invalid-edge]" on error
const char* pixi_piGpioEdgeToStr (Edge edge);

///	Get the state of @c pin using the /sys interface.
///	@return 1 if pin is exported, 0 if pin is not exported, -errno on error.
int pixi_piGpioSysGetPinState (uint pin, GpioState* state);

// TODO: work out how to get the python wrapper working
///	Get gpio pin states in range [0, @c count] using the /sys interface.
///	@return the number of exported gpios
int pixi_piGpioSysGetPinStates (GpioState* states, uint count);

///	Get the direction of a gpio pin using the /sys interface.
///	@return enum Direction on success, -errno on error
int pixi_piGpioSysGetPinDirection (uint pin);

///	Get the direction of a gpio pin using the /sys interface.
///	@return 0 or 1 on success, -errno on error
int pixi_piGpioSysGetActiveLow (uint pin);

///	Get the edge state of a gpio pin using the /sys interface.
///	@return enum Edge on success, -errno on error
int pixi_piGpioSysGetPinEdge (uint pin);

///	Read the value of a gpio pin using the /sys interface.
///	@return 0 or 1 on success, -errno on error
int pixi_piGpioSysReadPin (uint pin);

///	Write a value of to gpio pin using the /sys interface.
///	@return 0 or 1 on success, -errno on error
int pixi_piGpioSysWritePin (uint pin, uint value);

///	Enable the /sys interface of a gpio pin in the given @c direction.
///	@return >=0 on success, -errno on error
int pixi_piGpioSysExportPin (uint pin, Direction direction);

///	Disable the /sys interface of a gpio pin.
///	@return >=0 on success, -errno on error
int pixi_piGpioSysUnexportPin (uint pin);

///	Map the GPIO registers into memory, in preparation for using
///	non-/sys GPIO API.
int pixi_piGpioMapRegisters (void);

///	Unmap the GPIO registers from memory.
int pixi_piGpioUnmapRegisters (void);

///	Set the mode of a GPIO pin using the memory mapped registers.
///	@return 0 on success, -errno on error.
int pixi_piGpioSetPinMode (uint pin, Direction mode);

///	Get the mode of a GPIO pin using the memory mapped registers.
///	@return >=0 on success, -errno on error.
int pixi_piGpioGetPinMode (uint pin);

///	Like pixi_piGpioSetPinMode, but uses physical pin number.
int pixi_piGpioPhysSetPinMode (uint pin, Direction mode);

///	Like pixi_piGpioSetPinMode, but uses physical pin number.
int pixi_piGpioPhysGetPinMode (uint pin);

///	Read a GPIO pin value using the memory mapped registers.
///	@return 0 or 1 on success, -errno on error.
int pixi_piGpioReadPin (uint pin);

///	Like pixi_piGpioReadPin, but uses physical pin number.
int pixi_piGpioPhysReadPin (uint pin);

///	Write a GPIO pin value using the memory mapped registers.
///	@return 0 on success, -errno on error.
int pixi_piGpioWritePin (uint pin, int value);

///	Like pixi_piGpioWritePin, but uses physical pin number.
int pixi_piGpioPhysWritePin (uint pin, int value);

///	Get the state of @c pin using the memory mapped registers.
///	@return 0 on success, -errno on error
int pixi_piGpioGetPinState (uint pin, GpioState* state);

///	Like pixi_piGpioGetPinState, but uses physical pin number
int pixi_piGpioPhysGetPinState (uint pin, GpioState* state);

// TODO: work out how to get the python wrapper working
///	Get gpio pin states in range [0, @c count] using the memory mapped registers.
///	@return 0 on success, -errno on error
int pixi_piGpioPhysGetPinStates (GpioState* states, uint count);

///@} defgroup

LIBPIXI_END_DECLS

#endif // !defined libpixi_pi_gpio_h__included
