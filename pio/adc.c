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

#include <libpixi/pixi/adc.h>
#include <libpixi/pixi/simple.h>
#include <libpixi/util/string.h>
#include "common.h"
#include "log.h"
#include <stdio.h>

static int adcReadFn (const Command* command, uint argc, char* argv[])
{
	if (argc != 2)
		return commandUsageError (command);

	uint adcChannel = pixi_parseLong (argv[1]);

	adcOpenOrDie();
	int result = adcRead (adcChannel);
	adcClose();
	if (result < 0)
	{
		PIO_ERROR(-result, "ADC SPI read failed");
		return result;
	}

	printf ("%u\n", result);
	return 0;
}
static Command adcReadCmd =
{
	.name        = "adc-read",
	.description = "read an ADC channel",
	.usage       = "usage: %s CHANNEL",
	.function    = adcReadFn
};

static int adcMonitor (void)
{
	adcOpenOrDie();

	int result = 0;
	while (true)
	{
		printf ("\r");
		for (uint i = 0; i < PixiAdcMaxChannels; i++)
		{
			result = adcRead (i);
			if (result < 0)
			{
				PIO_ERROR(-result, "ADC SPI read failed");
				break;
			}
			printf ("%4u ", result);
		}
		fflush (stdout);
		usleep (100 * 1000);
	}
	adcClose();

	return result;
}


static int adcMonitorFn (const Command* command, uint argc, char* argv[])
{
	LIBPIXI_UNUSED(argv);
	if (argc != 1)
		return commandUsageError (command);

	return adcMonitor();
}
static Command adcMonitorCmd =
{
	.name        = "adc-monitor",
	.description = "Monitor all ADC channels",
	.usage       = "usage: %s",
	.function    = adcMonitorFn
};


static const Command* commands[] =
{
	&adcReadCmd,
	&adcMonitorCmd,
};

static CommandGroup pixiAdcGroup =
{
	.name      = "pixi-adc",
	.count     = ARRAY_COUNT(commands),
	.commands  = commands,
	.nextGroup = NULL
};

static void PIO_CONSTRUCTOR (1060) initGroup (void)
{
	addCommandGroup (&pixiAdcGroup);
}
