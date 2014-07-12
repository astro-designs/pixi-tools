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

#include <libpixi/libpixi.h>
#include <libpixi/pi/gpio.h>
#include <libpixi/util/file.h>
#include <libpixi/util/log.h>
#include <stddef.h>
#include <stdio.h>
#include <string.h>
#include <fcntl.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/user.h>
#include <unistd.h>

static void trim (char* buf)
{
	if (!buf || !buf[0])
		return;
	size_t len = strlen (buf);
	if (buf[len-1] == '\n')
		buf[len-1] = '\0';
}

static int readSysFileBool (const char* fname)
{
	char buf[10]; // room for the unexpected?

	int result = pixi_fileReadStr (fname, buf, sizeof (buf));
	if (result < 0)
	{
		LIBPIXI_ERROR(-result, "gpio file %s cannot be read", fname);
		return result;
	}

	if (buf[0] == '0')
		return 0;
	if (buf[0] == '1')
		return 1;

	LIBPIXI_LOG_ERROR("Unexpected value for gpio file %s: %s", fname, buf);
	return -EINVAL;
}

static int writeSysFileBool (const char* fname, bool value)
{
	const char* str = value ? "1\n" : "0\n";
	int result = pixi_fileWriteStr (fname, str);
	if (result < 0)
	{
		LIBPIXI_ERROR(-result, "gpio file %s cannot be written", fname);
		return result;
	}
	return 0;
}

int pixi_gpioStrToDirection (const char* direction)
{
	LIBPIXI_PRECONDITION_NOT_NULL(direction);

	if      (0 == strcmp ("in" , direction)) return DirectionIn;
	else if (0 == strcmp ("out", direction)) return DirectionOut;
	return -EINVAL;
}

const char* pixi_gpioDirectionToStr (Direction direction)
{
	switch (direction)
	{
	case DirectionIn : return "in";
	case DirectionOut: return "out";
	default          : return "[invalid-direction]";
	}
}

int pixi_gpioStrToEdge (const char* edge)
{
	LIBPIXI_PRECONDITION_NOT_NULL(edge);

	if      (0 == strcmp ("none"   , edge)) return EdgeNone;
	else if (0 == strcmp ("rising" , edge)) return EdgeRising;
	else if (0 == strcmp ("falling", edge)) return EdgeFalling;
	else if (0 == strcmp ("both"   , edge)) return EdgeBoth;
	return -EINVAL;
}

const char* pixi_gpioEdgeToStr (Edge edge)
{
	switch (edge)
	{
	case EdgeNone   : return "none";
	case EdgeRising : return "rising";
	case EdgeFalling: return "falling";
	case EdgeBoth   : return "both";
	default         : return "[invalid-edge]";
	}
}

int pixi_gpioSysGetPinDirection (uint gpio)
{
	char buf[40];
	char fname[256];

	sprintf (fname, "/sys/class/gpio/gpio%u/direction", gpio);
	int result = pixi_fileReadStr (fname, buf, sizeof (buf));
	if (result < 0)
		return result;

	trim (buf);
	result = pixi_gpioStrToDirection (buf);
	if (result < 0)
		LIBPIXI_LOG_ERROR("Unexpected value for gpio direction: %s is \"%s\"", fname, buf);
	return result;
}

int pixi_gpioSysGetPinEdge (uint gpio)
{
	char buf[40];
	char fname[256];

	sprintf (fname, "/sys/class/gpio/gpio%u/edge", gpio);
	int result = pixi_fileReadStr (fname, buf, sizeof (buf));
	if (result < 0)
		return result;
	trim (buf);
	int edge = pixi_gpioStrToEdge (buf);
	if (edge < 0)
		LIBPIXI_LOG_ERROR("Unexpected value for gpio edge: %s is \"%s\"", fname, buf);
	return edge;
}

int pixi_gpioSysReadPin (uint gpio)
{
	char fname[256];
	sprintf (fname, "/sys/class/gpio/gpio%u/value", gpio);
	return readSysFileBool (fname);
}

int pixi_gpioSysWritePin (uint gpio, uint value)
{
	char fname[256];
	sprintf (fname, "/sys/class/gpio/gpio%u/value", gpio);
	return writeSysFileBool (fname, 1 && value);
}

int pixi_gpioSysGetActiveLow (uint gpio)
{
	char fname[256];
	sprintf (fname, "/sys/class/gpio/gpio%u/active_low", gpio);
	return readSysFileBool (fname);
}

int pixi_gpioSysGetPinState (uint gpio, GpioState* state)
{
	LIBPIXI_PRECONDITION_NOT_NULL(state);

	memset (state, 0, sizeof (*state));

	int result;
	state->direction = result = pixi_gpioSysGetPinDirection (gpio);
	if (result == -ENOENT)
		return 0; // not exported
	else if (result < 0)
		return result; // some other error
	state->exported = true;

	state->edge = result = pixi_gpioSysGetPinEdge (gpio);
	if (result < 0)
		return result;

	state->value = result = pixi_gpioSysReadPin (gpio);
	if (result < 0)
		return result;

	state->activeLow = result = pixi_gpioSysGetActiveLow (gpio);
	if (result < 0)
		return result;

	return 1; // exported
}

int pixi_gpioSysGetPinStates (GpioState* states, uint count)
{
	LIBPIXI_PRECONDITION_NOT_NULL(states);

	uint exported = 0;
	for (uint pin = 0; pin < count; pin++)
	{
		pixi_gpioSysGetPinState (pin, &states[pin]);
		exported += states[pin].exported;
	}
	return exported;
}

int pixi_gpioSysExportPin (uint gpio, Direction direction)
{
	const char* dirStr = pixi_gpioDirectionToStr (direction);

	ssize_t result = pixi_fileWriteInt ("/sys/class/gpio/export", gpio);
	if (result < 0)
	{
		if (result == -EBUSY)
			LIBPIXI_LOG_TRACE("Export of gpio %d failed because it's already exported", gpio);
		return result;
	}

	char fname[256];
	sprintf (fname, "/sys/class/gpio/gpio%d/direction", gpio);
	return pixi_fileWriteStr (fname, dirStr);
}

int pixi_gpioSysUnexportPin (uint gpio)
{
	return pixi_fileWriteInt ("/sys/class/gpio/unexport", gpio);
}

/// Address map:
///
/// from BCM2835-ARM-Peripherals.pdf [page 6]: <blockquote>
///  Peripherals (at physical address 0x20000000 on) are mapped into the kernel virtual address
///   space starting at address 0xF2000000. Thus a peripheral advertised here at bus address
///   0x7Ennnnnn is available in the ARM kenel at virtual address 0xF2nnnnnn.
/// </blockquote>
/// However, /dev/mem is a map of physical memory, so for the 0x7E000000
/// described in the pdf, we do actually need 0x20000000

enum
{
	BcmPeripheralsBase     = 0x20000000,
	BcmGpioBase            =   0x200000 + BcmPeripheralsBase,
};

static const size_t PageSize = 4096;

/// GPIO Register map from BcmGpioBase offset. Taken from BCM2835-ARM-Peripherals.pdf [page 90/91]
struct BcmGpioRegisters
{
	uint32  functionSelect[6];             ///< Read/Write
	uint32  _reserved1;
	uint32  pinOutputSet[2];               ///< Write
	uint32  _reserved2;
	uint32  pinOutputClear[2];             ///< Write
	uint32  _reserved3;
	uint32  pinLevel[2];                   ///< Read
	uint32  _reserved4;
	uint32  pinEventDetectStatus[2];       ///< Read/Write
	uint32  _reserved5;
	uint32  pinRisingEdgeDetectEnable[2];  ///< Read/Write
	uint32  _reserved6;
	uint32  pinFallingEdgeDetectEnable[2]; ///< Read/Write
	uint32  _reserved7;
	uint32  pinHighDetectEnable[2];        ///< Read/Write
	uint32  _reserved8;
	uint32  pinLowDetectEnable[2];         ///< Read/Write
	uint32  _reserved9;
	uint32  pinAsyncRisingEdgeDetect[2];   ///< Read/Write
	uint32  _reserved10;
	uint32  pinAsyncFallingEdgeDetect[2];  ///< Read/Write
	uint32  _reserved11;
	uint32  pinPullUpDownEnable;           ///< Read/Write
	uint32  pinPullUpDownClock[2];         ///< Read/Write
	uint32  _reserved12[4];
	uint32  test;                          ///< Read/Write
};
static struct BcmGpioRegisters* gpioRegisters = NULL;

//	Values copied from wiringPi.c
static const int8 pinToGpioR1[64] =
{
	17, 18, 21, 22, 23, 24, 25,  4,
	 0,  1,  8,  7, 10,  9, 11, 14,
	15,
	    -1, -1, -1, -1, -1, -1, -1,
	-1, -1, -1, -1, -1, -1, -1, -1,
	-1, -1, -1, -1, -1, -1, -1, -1,
	-1, -1, -1, -1, -1, -1, -1, -1,
	-1, -1, -1, -1, -1, -1, -1, -1,
	-1, -1, -1, -1, -1, -1, -1, -1
};

//	Values copied from wiringPi.c
static const int8 pinToGpioR2[64] =
{
	17, 18, 27, 22, 23, 24, 25,  4,
	 2,  3,  8,  7, 10,  9, 11, 14,
	15, 28, 29, 30, 31,
	                    -1, -1, -1,
	-1, -1, -1, -1, -1, -1, -1, -1,
	-1, -1, -1, -1, -1, -1, -1, -1,
	-1, -1, -1, -1, -1, -1, -1, -1,
	-1, -1, -1, -1, -1, -1, -1, -1,
	-1, -1, -1, -1, -1, -1, -1, -1
};

//	We're using the Wiring Pi pin mappings. I've no idea if these
//	mappings are a good choice, although we should definitely support
//	compatibility. Perhaps allow client code to define its own mapping?
//	Mark?
static const int8* pinMap = pinToGpioR1;

static inline void* mapRegisters (int fd, off_t offset)
{
	void* mem = mmap (
		NULL, // request address
		PageSize,
		PROT_READ | PROT_WRITE,
		MAP_SHARED,
		fd,
		offset);
	if (mem == MAP_FAILED)
		LIBPIXI_ERRNO_ERROR("mmap of GPIO registers failed (/dev/mem at 0x%x", BcmGpioBase);
	return mem;
}

int pixi_gpioMapRegisters (void)
{
	int version = pixi_getPiBoardVersion();
	if (version == 2)
	{
		LIBPIXI_LOG_DEBUG("Using Pi version 2 GPIO mappings");
		pinMap = pinToGpioR2;
	}
	else
	{
		LIBPIXI_LOG_DEBUG("Using Pi version 1 GPIO mappings");
		pinMap = pinToGpioR1;
	}

	if (gpioRegisters)
		return 0; // ??

	// Statically validate some of the offsets
	LIBPIXI_STATIC_ASSERT(offsetof (struct BcmGpioRegisters, pinEventDetectStatus) == 0x40, "Validation of GPIO registers");
	LIBPIXI_STATIC_ASSERT(offsetof (struct BcmGpioRegisters, pinPullUpDownEnable) == 0x94, "Validation of GPIO registers");
	LIBPIXI_STATIC_ASSERT(offsetof (struct BcmGpioRegisters, test) == 0xB0, "Validation of GPIO registers");

	int fd = pixi_open ("/dev/mem", O_RDWR | O_SYNC, 0);
	if (fd < 0)
	{
		LIBPIXI_ERROR(-fd, "Could not open /dev/mem");
		return fd;
	}
	void* gpio = mapRegisters (fd, BcmGpioBase);
	int result = 0;
	if (gpio == MAP_FAILED)
		result = -errno;
	else
		gpioRegisters = gpio;
	pixi_close (fd);
	return result;
}

int pixi_gpioUnmapRegisters (void)
{
	if (!gpioRegisters)
		return 0;
	int result = munmap (gpioRegisters, PageSize);
	gpioRegisters = NULL;
	if (result < 0)
	{
		int err = errno;
		LIBPIXI_ERROR(err, "munmap of gpioRegisters failed");
		return -err;
	}
	return 0;
}

static inline uint pinToPhys (uint pin)
{
	return pinMap[pin & 63];
}

static inline void setPinRegisterBit (uint32* registers, uint pin)
{
	uint index = (pin & 32) && 1;
	uint bit   =  pin & 31;
	registers[index] = (1 << bit);
}

static inline bool getPinRegisterBit (uint32* registers, uint pin)
{
	uint index = (pin & 32) && 1;
	uint bit   =  pin & 31;
	uint bits = registers[index];
	bool value = true && (bits & (1 << bit));
	return value;
}

int pixi_gpioSetPinMode (uint pin, Direction mode)
{
	LIBPIXI_LOG_DEBUG("pixi_gpioSetPinMode (%u, %u)", pin, mode);
	LIBPIXI_PRECONDITION (pin < GpioNumPins);
	return pixi_gpioPhysSetPinMode (pinToPhys (pin), mode);
}

int pixi_gpioGetPinMode (uint pin)
{
	LIBPIXI_PRECONDITION (pin < GpioNumPins);
	return pixi_gpioPhysGetPinMode (pinToPhys (pin));
}

int pixi_gpioPhysSetPinMode (uint pin, Direction mode)
{
	LIBPIXI_LOG_DEBUG("pixi_gpioPhysSetPinMode (%u, %u)", pin, mode);
	LIBPIXI_PRECONDITION_NOT_NULL(gpioRegisters);
	LIBPIXI_PRECONDITION (pin < GpioNumPins);
	uint index =      pin / 10;
	uint shift = 3 * (pin % 10);
	uint mask  =         0x7  << shift;
	uint func  = (mode & 0x7) << shift;
	uint* reg = gpioRegisters->functionSelect + index;
	LIBPIXI_LOG_TRACE("Setting pin %u to mode 0x%x", pin, mode);
	*reg = (*reg & ~mask) | func;
	return 0;
}

int pixi_gpioPhysGetPinMode (uint pin)
{
	LIBPIXI_PRECONDITION_NOT_NULL(gpioRegisters);
	LIBPIXI_PRECONDITION (pin < GpioNumPins);
	uint index =      pin / 10;
	uint shift = 3 * (pin % 10);
	uint mask  =         0x7  << shift;
	uint* reg = gpioRegisters->functionSelect + index;
	return (*reg & mask) >> shift;
}

int pixi_gpioReadPin (uint pin)
{
	LIBPIXI_PRECONDITION (pin < GpioNumPins);
	return pixi_gpioPhysReadPin (pinToPhys (pin));
}

int pixi_gpioPhysReadPin (uint pin)
{
	LIBPIXI_PRECONDITION_NOT_NULL(gpioRegisters);
	LIBPIXI_PRECONDITION (pin < GpioNumPins);
	return getPinRegisterBit (gpioRegisters->pinLevel, pin);
}

int pixi_gpioWritePin (uint pin, int value)
{
	LIBPIXI_PRECONDITION (pin < GpioNumPins);
	return pixi_gpioPhysWritePin (pinToPhys (pin), value);
}

int pixi_gpioPhysWritePin (uint pin, int value)
{
	LIBPIXI_PRECONDITION_NOT_NULL(gpioRegisters);
	LIBPIXI_PRECONDITION (pin < GpioNumPins);
	uint32* registers = value ? gpioRegisters->pinOutputSet : gpioRegisters->pinOutputClear;
	setPinRegisterBit (registers, pin);
	return 0;
}

int pixi_gpioGetPinState (uint pin, GpioState* state)
{
	LIBPIXI_PRECONDITION (pin < GpioNumPins);
	return pixi_gpioPhysGetPinState (pinToPhys (pin), state);
}

int pixi_gpioPhysGetPinState (uint pin, GpioState* state)
{
	LIBPIXI_PRECONDITION_NOT_NULL(gpioRegisters);
	LIBPIXI_PRECONDITION (pin < GpioNumPins);
	LIBPIXI_PRECONDITION_NOT_NULL(state);

	memset (state, 0, sizeof (*state));
	state->direction = pixi_gpioPhysGetPinMode (pin);
	state->value     = pixi_gpioPhysReadPin (pin);
	// TODO: the other fields
	return 0;
}

int pixi_gpioPhysGetPinStates (GpioState* states, uint count)
{
	LIBPIXI_PRECONDITION_NOT_NULL(states);

	for (uint pin = 0; pin < count; pin++)
	{
		int result = pixi_gpioPhysGetPinState (pin, &states[pin]);
		if (result < 0)
			return result;
	}
	return 0;
}

