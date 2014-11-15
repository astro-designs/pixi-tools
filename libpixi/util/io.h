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

#ifndef libpixi_util_io_h__included
#define libpixi_util_io_h__included


#include <libpixi/common.h>

LIBPIXI_BEGIN_DECLS

///@defgroup util_io libpixi IO utilities
///@{

/// Disable terminal line-buffering
/// This can screw up your terminal, use the 'reset' command to fix it.
/// @param fd tty file descriptor
int pixi_ttyInputRaw (int fd);

/// Enable terminal line-buffering
/// @param fd tty file descriptor
int pixi_ttyInputNormal (int fd);

/// Check if locale text encoding is set to UTF-8.
bool pixi_isLocaleEncodingUtf8 (void);

///@} defgroup

LIBPIXI_END_DECLS

#endif // !defined libpixi_util_io_h__included
