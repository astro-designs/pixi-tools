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
#include <poll.h>
#include <stddef.h>
#include <stdio.h>
#include <string.h>
#include <fcntl.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/user.h>
#include <unistd.h>

const char sysGpio_export[]        = "/sys/class/gpio/export";
const char sysGpio_unexport[]      = "/sys/class/gpio/unexport";
const char sysGpioPin_value[]      = "/sys/class/gpio/gpio%u/value";
const char sysGpioPin_active_low[] = "/sys/class/gpio/gpio%u/active_low";
const char sysGpioPin_direction[]  = "/sys/class/gpio/gpio%u/direction";
const char sysGpioPin_edge[]       = "/sys/class/gpio/gpio%u/edge";


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

int pixi_piGpioStrToDirection (const char* direction)
{
	LIBPIXI_PRECONDITION_NOT_NULL(direction);

	if      (0 == strcmp ("in" , direction)) return DirectionIn;
	else if (0 == strcmp ("out", direction)) return DirectionOut;
	return -EINVAL;
}

const char* pixi_piGpioDirectionToStr (Direction direction)
{
	switch (direction)
	{
	case DirectionIn : return "in";
	case DirectionOut: return "out";
	default          : return "[invalid-direction]";
	}
}

int pixi_piGpioStrToEdge (const char* edge)
{
	LIBPIXI_PRECONDITION_NOT_NULL(edge);

	if      (0 == strcmp ("none"   , edge)) return EdgeNone;
	else if (0 == strcmp ("rising" , edge)) return EdgeRising;
	else if (0 == strcmp ("falling", edge)) return EdgeFalling;
	else if (0 == strcmp ("both"   , edge)) return EdgeBoth;
	return -EINVAL;
}

const char* pixi_piGpioEdgeToStr (Edge edge)
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

int pixi_piGpioSysGetPinDirection (uint gpio)
{
	char buf[40];
	char fname[256];

	sprintf (fname, sysGpioPin_direction, gpio);
	int result = pixi_fileReadStr (fname, buf, sizeof (buf));
	if (result < 0)
		return result;

	trim (buf);
	result = pixi_piGpioStrToDirection (buf);
	if (result < 0)
		LIBPIXI_LOG_ERROR("Unexpected value for gpio direction: %s is \"%s\"", fname, buf);
	return result;
}

int pixi_piGpioSysGetPinEdge (uint gpio)
{
	char buf[40];
	char fname[256];

	sprintf (fname, sysGpioPin_edge, gpio);
	int result = pixi_fileReadStr (fname, buf, sizeof (buf));
	if (result < 0)
		return result;
	trim (buf);
	int edge = pixi_piGpioStrToEdge (buf);
	if (edge < 0)
		LIBPIXI_LOG_ERROR("Unexpected value for gpio edge: %s is \"%s\"", fname, buf);
	return edge;
}

int pixi_piGpioSysSetPinEdge (uint pin, Edge edge)
{
	const char* edgeStr = pixi_piGpioEdgeToStr (edge);
	char fname[256];
	sprintf (fname, sysGpioPin_edge, pin);

	int result = pixi_fileWriteStr (fname, edgeStr);
	if (result < 0)
		LIBPIXI_LOG_ERROR("Error writing [%s] to [%s]", edgeStr, fname);
	return result;
}

int pixi_piGpioSysReadPin (uint gpio)
{
	char fname[256];
	sprintf (fname, sysGpioPin_value, gpio);
	return readSysFileBool (fname);
}

int pixi_piGpioSysWritePin (uint gpio, uint value)
{
	char fname[256];
	sprintf (fname, sysGpioPin_value, gpio);
	return writeSysFileBool (fname, 1 && value);
}

int pixi_piGpioSysGetActiveLow (uint gpio)
{
	char fname[256];
	sprintf (fname, sysGpioPin_active_low, gpio);
	return readSysFileBool (fname);
}

int pixi_piGpioSysGetPinState (uint gpio, GpioState* state)
{
	LIBPIXI_PRECONDITION_NOT_NULL(state);

	memset (state, 0, sizeof (*state));

	int result;
	state->direction = result = pixi_piGpioSysGetPinDirection (gpio);
	if (result == -ENOENT)
		return 0; // not exported
	else if (result < 0)
		return result; // some other error
	state->exported = true;

	state->edge = result = pixi_piGpioSysGetPinEdge (gpio);
	if (result < 0)
		return result;

	state->value = result = pixi_piGpioSysReadPin (gpio);
	if (result < 0)
		return result;

	state->activeLow = result = pixi_piGpioSysGetActiveLow (gpio);
	if (result < 0)
		return result;

	return 1; // exported
}

int pixi_piGpioSysGetPinStates (GpioState* states, uint count)
{
	LIBPIXI_PRECONDITION_NOT_NULL(states);

	uint exported = 0;
	for (uint pin = 0; pin < count; pin++)
	{
		pixi_piGpioSysGetPinState (pin, &states[pin]);
		exported += states[pin].exported;
	}
	return exported;
}

int pixi_piGpioSysExportPin (uint gpio, Direction direction)
{
	const char* dirStr = pixi_piGpioDirectionToStr (direction);

	ssize_t result = pixi_fileWriteInt (sysGpio_export, gpio);
	if (result < 0)
	{
		if (result == -EBUSY)
			LIBPIXI_LOG_TRACE("Export of gpio %d failed because it's already exported", gpio);
		return result;
	}

	char fname[256];
	sprintf (fname, sysGpioPin_direction, gpio);
	return pixi_fileWriteStr (fname, dirStr);
}

int pixi_piGpioSysUnexportPin (uint gpio)
{
	return pixi_fileWriteInt (sysGpio_unexport, gpio);
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

int pixi_piGpioInit (void)
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
	return version;
}

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

int pixi_piGpioMapRegisters (void)
{
	if (gpioRegisters)
		return 0; // ??

	pixi_piGpioInit();

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

int pixi_piGpioUnmapRegisters (void)
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

uint pixi_piGpioMapPinToPhys (uint pin)
{
	return pinToPhys (pin);
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

int pixi_piGpioSetPinMode (uint pin, Direction mode)
{
	LIBPIXI_LOG_DEBUG("pixi_piGpioSetPinMode (%u, %u)", pin, mode);
	LIBPIXI_PRECONDITION (pin < GpioNumPins);
	return pixi_piGpioPhysSetPinMode (pinToPhys (pin), mode);
}

int pixi_piGpioGetPinMode (uint pin)
{
	LIBPIXI_PRECONDITION (pin < GpioNumPins);
	return pixi_piGpioPhysGetPinMode (pinToPhys (pin));
}

int pixi_piGpioPhysSetPinMode (uint pin, Direction mode)
{
	LIBPIXI_LOG_DEBUG("pixi_piGpioPhysSetPinMode (%u, %u)", pin, mode);
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

int pixi_piGpioPhysGetPinMode (uint pin)
{
	LIBPIXI_PRECONDITION_NOT_NULL(gpioRegisters);
	LIBPIXI_PRECONDITION (pin < GpioNumPins);
	uint index =      pin / 10;
	uint shift = 3 * (pin % 10);
	uint mask  =         0x7  << shift;
	uint* reg = gpioRegisters->functionSelect + index;
	return (*reg & mask) >> shift;
}

int pixi_piGpioReadPin (uint pin)
{
	LIBPIXI_PRECONDITION (pin < GpioNumPins);
	return pixi_piGpioPhysReadPin (pinToPhys (pin));
}

int pixi_piGpioPhysReadPin (uint pin)
{
	LIBPIXI_PRECONDITION_NOT_NULL(gpioRegisters);
	LIBPIXI_PRECONDITION (pin < GpioNumPins);
	return getPinRegisterBit (gpioRegisters->pinLevel, pin);
}

int pixi_piGpioWritePin (uint pin, int value)
{
	LIBPIXI_PRECONDITION (pin < GpioNumPins);
	return pixi_piGpioPhysWritePin (pinToPhys (pin), value);
}

int pixi_piGpioPhysWritePin (uint pin, int value)
{
	LIBPIXI_PRECONDITION_NOT_NULL(gpioRegisters);
	LIBPIXI_PRECONDITION (pin < GpioNumPins);
	uint32* registers = value ? gpioRegisters->pinOutputSet : gpioRegisters->pinOutputClear;
	setPinRegisterBit (registers, pin);
	return 0;
}

int pixi_piGpioGetPinState (uint pin, GpioState* state)
{
	LIBPIXI_PRECONDITION (pin < GpioNumPins);
	return pixi_piGpioPhysGetPinState (pinToPhys (pin), state);
}

int pixi_piGpioPhysGetPinState (uint pin, GpioState* state)
{
	LIBPIXI_PRECONDITION_NOT_NULL(gpioRegisters);
	LIBPIXI_PRECONDITION (pin < GpioNumPins);
	LIBPIXI_PRECONDITION_NOT_NULL(state);

	memset (state, 0, sizeof (*state));
	state->direction = pixi_piGpioPhysGetPinMode (pin);
	state->value     = pixi_piGpioPhysReadPin (pin);
	// TODO: the other fields
	return 0;
}

int pixi_piGpioPhysGetPinStates (GpioState* states, uint count)
{
	LIBPIXI_PRECONDITION_NOT_NULL(states);

	for (uint pin = 0; pin < count; pin++)
	{
		int result = pixi_piGpioPhysGetPinState (pin, &states[pin]);
		if (result < 0)
			return result;
	}
	return 0;
}

int pixi_piGpioPhysOpenPin (uint pin)
{
	LIBPIXI_PRECONDITION (pin < GpioNumPins);
	int result = pixi_piGpioSysExportPin (pin, DirectionIn);
	if (result < 0 && result != -EBUSY) // EBUSY if already exported
	{
		LIBPIXI_ERROR(-result, "Failed to export gpio pin for interrupt use");
		return result;
	}
	char fname[256];
	sprintf (fname, sysGpioPin_value, pin);
	result = pixi_open (fname, O_RDONLY | O_NONBLOCK, 0);
	if (result < 0)
	{
		LIBPIXI_ERROR(-result, "Failed to open gpio pin for interrupt use");
		return result;
	}
	return result;
}

int pixi_piGpioOpenPin (uint pin)
{
	uint physPin = pinToPhys (pin);
	return pixi_piGpioPhysOpenPin (physPin);
}

int pixi_piGpioWait (int fileDesc, int timeout)
{
	LIBPIXI_PRECONDITION (fileDesc >= 0);
	struct pollfd pol = {
		.fd      = fileDesc,
		.events  = POLLPRI,
		.revents = 0
	};
	int result = poll (&pol, 1, timeout);
	if (result < 0)
	{
		LIBPIXI_ERROR(-result, "poll() failed in pixi_piGpioWait()");
		return result;
	}
	if (result == 0)
		return 0; // timed out
	// TODO: maybe check .revents?

	// clear the interrupt, and read the value
	char value;
	do {
		result = pread (fileDesc, &value, 1, 0);
	} while (result < 0 && errno == EINTR);

	if (result < 1)
	{
		if (result < 0)
			LIBPIXI_ERROR(-result, "Failed to read GPIO value after interrupt");
		else
		{
			LIBPIXI_LOG_ERROR("Short read of GPIO value after interrupt");
			result = -EIO;
		}
		return result;
	}
	result = 0x10; // interrupt received
	result |= (value == '1'); // put value in bit zero
	return result;
}
