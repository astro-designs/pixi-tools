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

#ifndef Command_h__included
#define Command_h__included


#include <pio/common.h>
#include <libpixi/libpixi.h>


typedef struct Command Command;
/// @return >=0 on success, -errno on error
typedef int (*CommandFn) (const Command* command, uint, char* []);

struct Command
{
	const char*  name;        ///< name of command
	const char*  description; ///< one line description of command
	const char*  usage;       ///< detailed usage message
	CommandFn    function;    ///< implementation
};

int commandUsageError (const Command* command);

typedef struct CommandGroup
{
	const char*                 name;     ///< name of the command group
	uint                        count;    ///< size of @c commands array
	const Command**             commands; ///< array of commands
	const struct CommandGroup*  nextGroup;///< internal pointer to next command group
} CommandGroup;

extern CommandGroup gpioGroup;

int addCommandGroup (CommandGroup* group);


#endif // !defined Command_h__included
