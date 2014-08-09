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

#include "common.h"
#include "log.h"
#include <libpixi/pixi/lcd.h>
#include <libpixi/pixi/spi.h>
#include <libpixi/util/string.h>

static int lcdInitFn (const Command* command, uint argc, char* argv[])
{
	LIBPIXI_UNUSED(argv);

	if (argc != 1)
		return commandUsageError (command);

	int result = pixi_lcdOpen();
	if (result < 0)
		return result;

	result = pixi_lcdInit();
	pixi_closePixi();
	return result;
}
static Command lcdInitCmd =
{
	.name        = "lcd-init",
	.description = "initialise the LCD",
	.usage       = "usage: %s",
	.function    = lcdInitFn
};

static int lcdBrightFn (const Command* command, uint argc, char* argv[])
{
	if (argc != 2)
		return commandUsageError (command);

	int result = pixi_lcdOpen();
	if (result < 0)
		return result;

	uint brightness = pixi_parseLong (argv[1]);
	result = pixi_lcdSetBrightness (brightness);
	pixi_closePixi();
	return result;
}
static Command lcdBrightCmd =
{
	.name        = "lcd-bright",
	.description = "set the LCD brightness",
	.usage       = "usage: %s BRIGHTNESS",
	.function    = lcdBrightFn
};

static int lcdClearFn (const Command* command, uint argc, char* argv[])
{
	LIBPIXI_UNUSED(argv);

	if (argc != 1)
		return commandUsageError (command);

	int result = pixi_lcdOpen();
	if (result < 0)
		return result;

	result = pixi_lcdClear();
	pixi_closePixi();
	return result;
}
static Command lcdClearCmd =
{
	.name        = "lcd-clear",
	.description = "clear the LCD text",
	.usage       = "usage: %s",
	.function    = lcdClearFn
};

static int lcdPosFn (const Command* command, uint argc, char* argv[])
{
	if (argc != 3)
		return commandUsageError (command);

	int result = pixi_openPixi();
	if (result < 0)
		return result;

	uint x = pixi_parseLong (argv[1]);
	uint y = pixi_parseLong (argv[2]);
	result = pixi_lcdSetCursorPos (x, y);
	pixi_closePixi();
	return result;
}
static Command lcdPosCmd =
{
	.name        = "lcd-pos",
	.description = "set the LCD cursor position",
	.usage       = "usage: %s X Y",
	.function    = lcdPosFn
};

static int lcdWriteFn (const Command* command, uint argc, char* argv[])
{
	if (argc != 2)
		return commandUsageError (command);

	int result = pixi_openPixi();
	if (result < 0)
		return result;

	result = pixi_lcdWriteStr (argv[1]);
	pixi_closePixi();
	return result;
}
static Command lcdWriteCmd =
{
	.name        = "lcd-write",
	.description = "write (append) a string to the LCD",
	.usage       = "usage: %s STRING",
	.function    = lcdWriteFn
};

static int lcdPrintFn (const Command* command, uint argc, char* argv[])
{
	if (argc != 4)
		return commandUsageError (command);

	int result = pixi_lcdOpen();
	if (result < 0)
		return result;

	uint x = pixi_parseLong (argv[1]);
	uint y = pixi_parseLong (argv[2]);
	pixi_lcdClear();
	pixi_lcdSetCursorPos (x, y);
	result = pixi_lcdWriteStr (argv[3]);
	pixi_closePixi();
	return result;
}
static Command lcdPrintCmd =
{
	.name        = "lcd-print",
	.description = "print a string on the LCD at a given position",
	.usage       = "usage: %s X Y STRING",
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

static void PIO_CONSTRUCTOR (1910) initGroup (void)
{
	addCommandGroup (&pixiLcdGroup);
}
