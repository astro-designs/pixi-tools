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

#ifndef libpixi_util_command_h__included
#define libpixi_util_command_h__included


#include <libpixi/common.h>
#include <libpixi/libpixi.h>
#include <libpixi/util/app-log.h>
#include <stdlib.h>

LIBPIXI_BEGIN_DECLS

///@defgroup util_command libpixi command line utilities
///@{

enum CommandFlags
{
	CmdHidden = 0x01 ///< Don't display the command/group in help
};

typedef struct Command Command;
/// @return >=0 on success, -errno on error
typedef int (*CommandFn) (const Command* command, uint, char* []);

///	Describes a program command
struct Command
{
	const char*  name;        ///< name of command
	const char*  description; ///< one line description of command
	const char*  usage;       ///< detailed usage message
	CommandFn    function;    ///< implementation
	int          flags;
	intptr       _reserved[3];
};

///	Display command usage error for @a command
int pixi_commandUsageError (const Command* command);
///	Wrapper for @ref pixi_commandUsageError
static inline int commandUsageError (const Command* command) {
	return pixi_commandUsageError (command);
}

///	Groups program commands together
typedef struct CommandGroup
{
	const char*                 name;     ///< name of the command group
	uint                        count;    ///< size of @c commands array
	const Command**             commands; ///< array of commands
	const struct CommandGroup*  nextGroup;///< internal pointer to next command group
	int                         flags;
	intptr                      _reserved[3];
} CommandGroup;

///	Add a command group to the global list
int pixi_addCommandGroup (CommandGroup* group);
///	Wrapper for @ref pixi_addCommandGroup
static inline int addCommandGroup (CommandGroup* group) {
	return pixi_addCommandGroup (group);
}

///	Invoke the command specified on the command line, or process --help/--version commands.
///	@param libpixiVersion   must pass LIBPIXI_VERSION_INT
///	@param info             description of this application (NULL is allowed)
///	@param argc             argc as passed to main()
///	@param argv             argv as passed to main()
///	@return program exit code
int pixi_commandMain (int libpixiVersion, const ProgramInfo* info, int argc, char* argv[]);

///	Wrapper for pixi_commandMain
///	Invoke the command specified on the command line, or process --help/--version commands.
///	@param info             description of this application (NULL is allowed)
///	@param argc             argc as passed to main()
///	@param argv             argv as passed to main()
///	@return program exit code
static inline int pixi_main (const ProgramInfo* info, int argc, char* argv[]) {
	return pixi_commandMain (LIBPIXI_VERSION_INT, info, argc, argv);
}

#define LIBPIXI_COMMAND_GROUP(priority) LIBPIXI_CONSTRUCTOR(10000 + priority)

///@} defgroup

LIBPIXI_END_DECLS

#endif // !defined libpixi_util_command_h__included
