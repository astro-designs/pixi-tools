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
#include <errno.h>
#include <locale.h>
#include <stdlib.h>
#include <stdio.h>
#include <strings.h>
#include "Command.h"
#include "log.h"


LogLevel pio_logLevel = LogLevelAll;

static CommandGroup* groups = &gpioGroup;
static CommandGroup* lastGroup = &gpioGroup;

int addCommandGroup (CommandGroup* group)
{
	PIO_PRECONDITION_NOT_NULL(group);
	PIO_PRECONDITION_NOT_NULL(groups);
	PIO_PRECONDITION_NOT_NULL(lastGroup);

	group->nextGroup = NULL;
	lastGroup->nextGroup = group;
	lastGroup = group;
	return 0;
}

int commandUsageError (const Command* command)
{
	PIO_LOG_ERROR (command->usage, command->name);
	return -EINVAL;
}

static void displayHelp (const char* program, bool verbose)
{
	printf (
		"pio: a program to interface with Raspberry Pi hardware\n"
		"usage: %s COMMAND [ARGS]\n"
		"\n"
		"  -h, --help            display this help, then exit\n"
		"      --version         display version information, then exit\n"
		"\n",
		program
		);
	for (const CommandGroup* group = groups; group != NULL; group = group->nextGroup)
	{
		printf ("%s commands:\n", group->name);
		for (uint i = 0; i < group->count; i++)
		{
			const Command* cmd = group->commands[i];
			printf ("  %s%-20s%s  %s\n", pixi_stdoutBold, cmd->name, pixi_stdoutReset, cmd->description);
			if (verbose && cmd->usage)
			{
				printf ("    ");
				printf (cmd->usage, cmd->name, cmd->description);
				printf ("\n\n");
			}
		}
	}
}

int main (int argc, char* argv[])
{
	setlocale (LC_ALL, "");

	if (argc < 2)
	{
		displayHelp (argv[0], false);
		return 1;
	}

	const char* command = argv[1];

	if (
		0 == strcasecmp (command, "-h") ||
		0 == strcasecmp (command, "--help") ||
		0 == strcasecmp (command, "help")
		)
	{
		displayHelp (argv[0], false);
		return 0;
	}

	if (
		0 == strcasecmp (command, "--help-all") ||
		0 == strcasecmp (command, "help-all")
		)
	{
		displayHelp (argv[0], true);
		return 0;
	}

	if (
		0 == strcasecmp (command, "--version") ||
		0 == strcasecmp (command, "version")
		)
	{
		printf ("pio version %s\n", pixi_getLibVersion());
		return 0;
	}

	pio_logLevel = pixi_strToLogLevel (getenv ("PIO_LOG_LEVEL"), LogLevelInfo);

	int result = pixi_initLib();
//	if (result < 0)
//		return 255;

	for (const CommandGroup* group = groups; group != NULL; group = group->nextGroup)
	{
		for (uint i = 0; i < group->count; i++)
		{
			const Command* cmd = group->commands[i];
			if (0 == strcasecmp (command, cmd->name))
			{
				result = cmd->function (cmd, argc - 1, argv + 1);
				return result < 0 ? 2 : 0;
			}
		}
	}
	PIO_LOG_ERROR("Unknown command: %s", command);
	return 1;
}
