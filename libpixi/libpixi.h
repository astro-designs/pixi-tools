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

#ifndef libpixi_libpixi_h__included
#define libpixi_libpixi_h__included


#include <libpixi/common.h>
#include <libpixi/version.h>

LIBPIXI_BEGIN_DECLS

///@mainpage libpixi documentation
///
/// libpixi is a library providing access to PiXi-200 and Raspberry Pi
/// hardware. @par
///
/// The library is divided into three main sections - util, pi and pixi -
/// the code/headers each found in the respective sub-directory of libpixi.
/// @par
///
/// The util section includes @ref util_log, @ref util_file and @ref
/// util_string modules. @par
///
/// The pi section includes @ref PiGpio, @ref PiSpi and @ref PiI2C modules.
///
/// The pixi section includes many modules, particularly including @ref
/// PixiFpga, @ref PixiGpio and @ref PiXiSpi modules. @par
///
/// Function names that are part of the ABI (public symbols in the shared
/// library) are all prefixed with pixi_, generally followed by the module
/// name, for example @ref pixi_fpgaLoadFile, @ref pixi_gpioWritePin. Names
/// that are not part of the ABI, such as inline functions, structs and
/// enums, omit the pixi_ prefix (the layout of a struct is usually part of
/// the ABI, but the name is not). @par
///
/// The code is written in C99. The header files are C89 compatible, except
/// for the logging macros, which are GNU89 compatible (the default for the
/// gcc compiler), so an application does *not* have to use C99 mode to use
/// the library. @par
///

///@defgroup libpixi libpixi core interface
///@{

///	Initialise the library.
///	@param expectedVersion must pass LIBPIXI_VERSION_INT
int pixi_initLib (int expectedVersion);

///	Describes a library/application. All fields are optional.
typedef struct ProgramInfo
{
	const char*   name;
	const char*   description;
	const char*   version;
	int           versionInt;
	const char*   buildVersion;
	const char*   buildDate;
	const char*   buildTime;
	intptr        _reserved[4];
} ProgramInfo;

///	Get the runtime library version
const char* pixi_getLibVersion (void);

///	Get the runtime library version
const ProgramInfo* pixi_getLibInfo (void);

///	Return the revision string from /proc/cpuinfo
///	@see pixi_getPiBoardVersion()
const char* pixi_getPiBoardRevision (void);

///	Return the board version based on @ref pixi_getPiBoardRevision()
int pixi_getPiBoardVersion (void);

///@} defgroup

LIBPIXI_END_DECLS

#endif // !defined libpixi_libpixi_h__included
