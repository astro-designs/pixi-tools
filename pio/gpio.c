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

#include <libpixi/pi/gpio.h>
#include <libpixi/util/file.h>
#include <libpixi/util/string.h>
#include "common.h"
#include "log.h"
#include <stdio.h>
#include <stdlib.h>

static int gpioPinsFn (const Command* command, uint argc, char* argv[])
{
	LIBPIXI_UNUSED(argv);
	if (argc > 1)
		return commandUsageError (command);

	int result = pixi_piGpioMapRegisters();
	if (result < 0)
	{
		PIO_ERROR (-result, "could not map gpio registers");
		return result;
	}
	GpioState states[GpioNumPins];
	result = pixi_piGpioPhysGetPinStates (states, ARRAY_COUNT(states));
	if (result < 0)
	{
		PIO_ERROR (-result, "could not get gpio pin states");
		return result;
	}
	const char header[] = "gpio | mode | value\n";
	const char row[]    = "%4u | %4x | %5x\n";
	printf (header);
	for (uint gpio = 0; gpio < ARRAY_COUNT(states); gpio++)
	{
		const GpioState* state = &states[gpio];
		printf (row,
			gpio,
			state->direction,
			state->value
			);
	}
	return 0;
}
static Command gpioPinsCmd =
{
	.name        = "gpio-pins",
	.description = "display state of gpio pins",
	.usage       = "usage: %s",
	.function    = gpioPinsFn
};

static int listExportsFn (const Command* command, uint argc, char* argv[])
{
	LIBPIXI_UNUSED(argv);
	if (argc > 1)
		return commandUsageError (command);

	GpioState states[64];
	int result = pixi_piGpioSysGetPinStates (states, ARRAY_COUNT(states));
	printf ("%d gpio exports\n", result);
	const char header[] = "gpio | direction | value |    edge | activeLow\n";
	const char row[]    = "%4u | %9s | %5d | %7s | %9d\n";
	printf (header);
	for (uint gpio = 0; gpio < ARRAY_COUNT(states); gpio++)
	{
		const GpioState* state = &states[gpio];
		if (!state->exported)
			continue;
		printf (row,
			gpio,
			pixi_piGpioDirectionToStr (state->direction),
			state->value,
			pixi_piGpioEdgeToStr (state->edge),
			state->activeLow
			);
	}
	return 0;
}
static Command listExportsCmd =
{
	.name        = "exports",
	.description = "display state of gpios",
	.usage       = "usage: %s",
	.function    = listExportsFn
};


static int exportGpioFn (const Command* command, uint argc, char* argv[])
{
	LIBPIXI_UNUSED(argv);
	if (argc != 3)
		return commandUsageError (command);

	int gpio = atoi (argv[1]);
	const char* directionStr = argv[2];
	Direction direction = pixi_piGpioStrToDirection (directionStr);
	if ((int) direction < 0)
	{
		PIO_LOG_ERROR ("[%s] is not a valid direction", directionStr);
		return -EINVAL;
	}
	int result = pixi_piGpioSysExportPin (gpio, direction);
	if (result < 0)
	{
		PIO_ERROR (-result, "Export of gpio %d (%s) failed", gpio, directionStr);
		return -EINVAL;
	}
	// TODO: change owner of exported file
	// TODO: 'force' mode. Force re-export if already exported (EBUSY)
	return 0;
}
static Command exportGpioCmd =
{
	.name        = "export",
	.description = "export a gpio pin",
	.usage       = "usage: %s PIN DIRECTION",
	.function    = exportGpioFn
};


static int unexportGpioFn (const Command* command, uint argc, char* argv[])
{
	LIBPIXI_UNUSED(argv);
	if (argc != 2)
		return commandUsageError (command);

	int gpio = atoi (argv[1]);
	int result = pixi_piGpioSysUnexportPin (gpio);
	if (result < 0)
	{
		PIO_ERROR (-result, "Unexport of gpio %d failed", gpio);
		return -EINVAL;
	}
	return 0;
}
static Command unexportGpioCmd =
{
	.name        = "unexport",
	.description = "unexport a gpio pin",
	.usage       = "usage: %s PIN",
	.function    = unexportGpioFn
};


static int monitorPiGpio (uint pin)
{
	uint sysPin = pixi_piGpioMapWiringPiToChip (pin);
	APP_LOG_INFO("Pin %u maps to /sys/ pin %u", pin, sysPin);
	int fd = pixi_piGpioPhysOpenPin (sysPin);
	if (fd < 0)
	{
		APP_ERROR(-fd, "Failed to open pin");
		return fd;
	}
	int result = pixi_piGpioSysSetPinEdge (sysPin, EdgeBoth);
	if (result < 0)
		APP_ERROR(-result, "Failed to set pin edge mode");
	else
	{
		const int timeout = 60000;
		while (true)
		{
			result = pixi_piGpioWait (fd, timeout);
			if (result < 0)
			{
				APP_ERROR(-result, "Wait for interrupt failed");
				break;
			}
			else if (result == 0)
				APP_LOG_DEBUG("Interrupt timeout");
			else
			{
				uint value = result & 0x1;
				char timeStr[40] = "";
				pixi_formatCurTime (timeStr, sizeof (timeStr));
				printf ("%s value: %u\n", timeStr, value);
			}
		}
	}
	pixi_close (fd);
	return result;
}
static int monitorPiGpioFn (const Command* command, uint argc, char* argv[])
{
	LIBPIXI_UNUSED(argv);
	if (argc != 2)
		return commandUsageError (command);

	int pin = pixi_parseLong (argv[1]);
	return monitorPiGpio (pin);
}
static Command monitorPiGpioCmd =
{
	.name        = "monitor-pi-gpio",
	.description = "Monitor value of Pi GPIO pin",
	.usage       = "usage: %s PIN",
	.function    = monitorPiGpioFn
};


static const Command* gpioCommands[] =
{
	&gpioPinsCmd,
	&listExportsCmd,
	&exportGpioCmd,
	&unexportGpioCmd,
	&monitorPiGpioCmd,
};

static CommandGroup gpioGroup =
{
	.name      = "gpio",
	.count     = ARRAY_COUNT(gpioCommands),
	.commands  = gpioCommands,
	.nextGroup = NULL
};

static void PIO_CONSTRUCTOR (1000) initGroup (void)
{
	addCommandGroup (&gpioGroup);
}
