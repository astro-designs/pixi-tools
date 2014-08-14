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

#include <libpixi/util/command.h>
#include <libpixi/util/log.h>
#include <errno.h>
#include <locale.h>
#include <stdlib.h>
#include <stdio.h>
#include <strings.h>

static CommandGroup* groups = NULL;
static CommandGroup* lastGroup = NULL;

int pixi_addCommandGroup (CommandGroup* group)
{
	LIBPIXI_PRECONDITION_NOT_NULL(group);

	group->nextGroup = NULL;
	if (groups)
		lastGroup->nextGroup = group;
	else
	{
		groups = group;
		lastGroup = group;
	}
	lastGroup = group;
	return 0;
}

int pixi_commandUsageError (const Command* command)
{
	LIBPIXI_LOG_ERROR (command->usage, command->name);
	return -EINVAL;
}

static void displayHelp (const char* description, const char* program, bool verbose)
{
	printf (
		"%s\n"
		"usage: %s COMMAND [ARGS]\n"
		"\n"
		"  -h, --help            display help, then exit\n"
		"      --help-all        display extended help, then exit\n"
		"      --version         display version information, then exit\n"
		"\n",
		description,
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

int pixi_main (const char* description, const char* version, int argc, char* argv[])
{
	setlocale (LC_ALL, "");

	if (argc < 2)
	{
		displayHelp (description, argv[0], false);
		return 1;
	}

	const char* command = argv[1];

	if (
		0 == strcasecmp (command, "-h") ||
		0 == strcasecmp (command, "--help") ||
		0 == strcasecmp (command, "help")
		)
	{
		displayHelp (description, argv[0], false);
		return 0;
	}

	if (
		0 == strcasecmp (command, "--help-all") ||
		0 == strcasecmp (command, "help-all")
		)
	{
		displayHelp (description, argv[0], true);
		return 0;
	}

	if (
		0 == strcasecmp (command, "--version") ||
		0 == strcasecmp (command, "version")
		)
	{
		printf ("libpixi version %s\n", pixi_getLibVersion());
		if (version)
			printf ("%s version %s\n", program_invocation_short_name, version);
		return 0;
	}

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
	LIBPIXI_LOG_ERROR("Unknown command: %s", command);
	return 1;
}