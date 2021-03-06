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
	intptr     _reserved[2];
} GpioState;

enum PinValue
{
	Low  = 0,
	High = 1
};

///	Map from a WiringPi GPIO pin number to the chip GPIO number.
uint pixi_piGpioMapWiringPiToChip (uint pin) LIBPIXI_PURE;

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

///	Get the direction of a gpio pin using the /sys interface.
///	@return enum Direction on success, -errno on error
int pixi_piGpioSysGetPinDirection (uint pin);

///	Get the direction of a gpio pin using the /sys interface.
///	@return 0 or 1 on success, -errno on error
int pixi_piGpioSysGetActiveLow (uint pin);

///	Get the edge mode of a gpio pin using the /sys interface.
///	@return enum Edge on success, -errno on error
int pixi_piGpioSysGetPinEdge (uint pin);

///	Set the edge mode of a gpio pin using the /sys interface.
///	@return 0 on success, -errno on error
int pixi_piGpioSysSetPinEdge (uint pin, Edge edge);

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
int pixi_piGpioChipSetPinMode (uint pin, Direction mode);

///	Get the mode of a GPIO pin using the memory mapped registers.
///	@return >=0 on success, -errno on error.
int pixi_piGpioChipGetPinMode (uint pin);

///	Read a GPIO pin value using the memory mapped registers.
///	@return 0 or 1 on success, -errno on error.
int pixi_piGpioChipReadPin (uint pin);

///	Write a GPIO pin value using the memory mapped registers.
///	@return 0 on success, -errno on error.
int pixi_piGpioChipWritePin (uint pin, int value);

///	Get the state of @c pin using the memory mapped registers.
///	@return 0 on success, -errno on error
int pixi_piGpioChipGetPinState (uint pin, GpioState* state);

///	Open a GPIO pin file descriptor for interrupt handling.
///	Will first ensure pin is exported to /sys/.
///	Close the file descriptor when finished.
///	@see pixi_piGpioWait()
///	@see pixi_piGpioSysSetPinEdge()
///	@return file descriptor on success, negative error number on error.
int pixi_piGpioChipOpenPin (uint pin);

///	Wait for an interrupt on @a fileDesc.
///	@param fileDesc a file descriptor opened with pixi_piGpioOpenPin.
///	@param timeout how long to wait (milliseconds), <0 for no timeout.
///	@return >0 on interrupt (bit zero holds new value), 0 on timeout, negative error number on error.
int pixi_piGpioWait (int fileDesc, int timeout);

///@} defgroup

LIBPIXI_END_DECLS

#endif // !defined libpixi_pi_gpio_h__included
