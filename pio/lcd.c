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

#include "Command.h"
#include "log.h"
#include <libpixi/pixi/lcd.h>
#include <libpixi/util/string.h>

static int lcdInitFn (uint argc, char*const*const argv)
{
	LIBPIXI_UNUSED(argv);

	if (argc != 1)
	{
		PIO_LOG_ERROR ("usage: %s", argv[0]);
		return -EINVAL;
	}
	LcdDevice device;
	initLcdDevice (&device);
	int result = pixi_lcdOpen (&device);
	if (result < 0)
		return result;

	result = pixi_lcdInit (&device);
	pixi_lcdClose (&device);
	return result;
}
static Command lcdInitCmd =
{
	.name        = "lcd-init",
	.description = "initialise the LCD",
	.function    = lcdInitFn
};

static int lcdBrightFn (uint argc, char*const*const argv)
{
	if (argc != 2)
	{
		PIO_LOG_ERROR ("usage: %s BRIGHTNESS", argv[0]);
		return -EINVAL;
	}
	LcdDevice device;
	initLcdDevice (&device);
	int result = pixi_lcdOpen (&device);
	if (result < 0)
		return result;

	uint brightness = pixi_parseLong (argv[1]);
	result = pixi_lcdSetBrightness (&device, brightness);
	pixi_lcdClose (&device);
	return result;
}
static Command lcdBrightCmd =
{
	.name        = "lcd-bright",
	.description = "set the LCD brightness",
	.function    = lcdBrightFn
};

static int lcdClearFn (uint argc, char*const*const argv)
{
	LIBPIXI_UNUSED(argv);

	if (argc != 1)
	{
		PIO_LOG_ERROR ("usage: %s", argv[0]);
		return -EINVAL;
	}
	LcdDevice device;
	initLcdDevice (&device);
	int result = pixi_lcdOpen (&device);
	if (result < 0)
		return result;

	result = pixi_lcdClear (&device);
	pixi_lcdClose (&device);
	return result;
}
static Command lcdClearCmd =
{
	.name        = "lcd-clear",
	.description = "clear the LCD text",
	.function    = lcdClearFn
};

static int lcdPosFn (uint argc, char*const*const argv)
{
	if (argc != 3)
	{
		PIO_LOG_ERROR ("usage: %s X Y", argv[0]);
		return -EINVAL;
	}
	LcdDevice device;
	initLcdDevice (&device);
	int result = pixi_lcdOpen (&device);
	if (result < 0)
		return result;

	uint x = pixi_parseLong (argv[1]);
	uint y = pixi_parseLong (argv[2]);
	result = pixi_lcdSetCursorPos (&device, x, y);
	pixi_lcdClose (&device);
	return result;
}
static Command lcdPosCmd =
{
	.name        = "lcd-pos",
	.description = "set the LCD cursor position",
	.function    = lcdPosFn
};

static int lcdWriteFn (uint argc, char*const*const argv)
{
	if (argc != 2)
	{
		PIO_LOG_ERROR ("usage: %s STRING", argv[0]);
		return -EINVAL;
	}
	LcdDevice device;
	initLcdDevice (&device);
	int result = pixi_lcdOpen (&device);
	if (result < 0)
		return result;

	result = pixi_lcdWriteStr (&device, argv[1]);
	pixi_lcdClose (&device);
	return result;
}
static Command lcdWriteCmd =
{
	.name        = "lcd-write",
	.description = "write (append) a string to the LCD",
	.function    = lcdWriteFn
};

static int lcdPrintFn (uint argc, char*const*const argv)
{
	if (argc != 4)
	{
		PIO_LOG_ERROR ("usage: %s X Y STRING", argv[0]);
		return -EINVAL;
	}
	LcdDevice device;
	initLcdDevice (&device);
	int result = pixi_lcdOpen (&device);
	if (result < 0)
		return result;

	uint x = pixi_parseLong (argv[1]);
	uint y = pixi_parseLong (argv[2]);
	pixi_lcdClear (&device);
	pixi_lcdSetCursorPos (&device, x, y);
	result = pixi_lcdWriteStr (&device, argv[3]);
	pixi_lcdClose (&device);
	return result;
}
static Command lcdPrintCmd =
{
	.name        = "lcd-print",
	.description = "print a string on the LCD at a given position",
	.function    = lcdPrintFn
};

static const Command* commands[] =
{
	&lcdInitCmd,
	&lcdBrightCmd,
	&lcdClearCmd,
	&lcdPosCmd,
	&lcdWriteCmd,
	&lcdPrintCmd
};


static CommandGroup pixiLcdGroup =
{
	.name      = "pixi-lcd",
	.count     = ARRAY_COUNT(commands),
	.commands  = commands,
	.nextGroup = NULL
};

static void PIO_CONSTRUCTOR (103) initGroup (void)
{
	addCommandGroup (&pixiLcdGroup);
}
