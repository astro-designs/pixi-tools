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

#include <libpixi/pi/i2c.h>
#include <libpixi/pixi/dac.h>
#include <libpixi/util/file.h>
#include <libpixi/util/string.h>
#include <stdio.h>
#include "common.h"
#include "log.h"


static int dacWrite (uint channel, int value)
{
	int i2c = pixi_i2cOpen (PixiDacChannel, PixiDacAddress);
	if (i2c < 0)
	{
		APP_ERROR(-i2c, "Failed to open DAC i2c device");
		return i2c;
	}
	int result = pixi_dacWriteValue (i2c, channel, value);

	pixi_close (i2c);

	return result;
}


static int dacWriteFn (const Command* command, uint argc, char* argv[])
{
	LIBPIXI_UNUSED(argv);
	if (argc != 3)
		return commandUsageError (command);

	uint channel = pixi_parseLong (argv[1]);
	int  value   = pixi_parseLong (argv[2]);

	return dacWrite (channel, value);
}

static Command dacWriteCmd =
{
	.name        = "dac-write",
	.description = "Write a value to a DAC channel",
	.usage       = "usage: %s CHANNEL VALUE",
	.function    = dacWriteFn
};


static const Command* commands[] =
{
	&dacWriteCmd,
};


static CommandGroup pixiDacGroup =
{
	.name      = "pixi-dac",
	.count     = ARRAY_COUNT(commands),
	.commands  = commands,
	.nextGroup = NULL
};

static void PIO_CONSTRUCTOR (1061) initGroup (void)
{
	addCommandGroup (&pixiDacGroup);
}
