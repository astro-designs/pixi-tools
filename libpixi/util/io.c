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

#include <libpixi/util/io.h>
#include <libpixi/util/log.h>
#include <termios.h>
#include <unistd.h>


int pixi_ttyInputRaw (int fd)
{
	LIBPIXI_PRECONDITION (fd >= 0);

	struct termios term;
	int result = tcgetattr (fd, &term);
	if (result < 0)
	{
		LIBPIXI_ERRNO_ERROR("tcgetattr failed");
		return result;
	}
	term.c_lflag &= ~(ICANON | ECHO);
	result = tcsetattr (fd, TCSAFLUSH, &term);
	if (result < 0)
		LIBPIXI_ERRNO_ERROR("tcsetattr failed when trying to disable line-buffering");
	return result;
}

int pixi_ttyInputNormal (int fd)
{
	LIBPIXI_PRECONDITION (fd >= 0);

	struct termios term;
	int result = tcgetattr (fd, &term);
	if (result < 0)
	{
		LIBPIXI_ERRNO_ERROR("tcgetattr failed");
		return result;
	}
	term.c_lflag |= (ICANON | ECHO);
	result = tcsetattr (fd, TCSAFLUSH, &term);
	if (result < 0)
		LIBPIXI_ERRNO_ERROR("tcsetattr failed when trying to enable line-buffering");
	return result;
}
