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

#include <libpixi/pixi/simple.h>
#include <libpixi/util/io.h>
#include <libpixi/util/string.h>
#include <ctype.h>
#include <poll.h>
#include <fcntl.h>
#include <stdio.h>
#include <termios.h>
#include <unistd.h>
#include "Command.h"
#include "log.h"


static int setupSerial (int serialFd)
{
	struct termios term;
	int result = tcgetattr (serialFd, &term);
	if (result < 0)
	{
		PIO_ERRNO_ERROR("tcgetattr of serial device failed");
		return result;
	}
	result = cfsetspeed (&term, B9600);
	if (result < 0)
		PIO_ERRNO_ERROR("cfsetspeed failed to set baud rate");

	result = tcsetattr (serialFd, TCSANOW, &term);
	if (result < 0)
		PIO_ERRNO_ERROR("tcsetattr failed to set baud rate");
	return result;
}

enum
{
	BufferLen  = 16,
	BufferMask = BufferLen - 1,
	DisplayChars = 40, // per line
	DisplayLines = 2
};

enum CmdState
{
	Ready,
	Escape,

	// ESC,0x13,0x13,0x13,0
	Sleep1,
	Sleep2,
	Sleep3,

	Beep, // ESC,BEL,duration

	// ESC,G,x,y
	GotoX,
	GotoY,

	LcdBright, // ESC,L,brightness
	KeyBright, // ESC,l,brightness

	LcdContrast // ESC,O,contrast
};

typedef struct State
{
	int       inputFd;
	int       serialFd;

	byte      outputBuf[BufferLen];
	size_t    outputSize;
	size_t    outputOffset;
	bool      asleep;
	int       cmdState;

	char      display[DisplayLines][DisplayChars+1];
	uint      xPos;
	uint      yPos;
} State;


static const char keyUp[]    = {0x1b, 0x5b, 0x41, 0};
static const char keyDown[]  = {0x1b, 0x5b, 0x42, 0};
static const char keyRight[] = {0x1b, 0x5b, 0x43, 0};
static const char keyLeft[]  = {0x1b, 0x5b, 0x44, 0};


static void writeDisplayChar (State* state, byte ch)
{
	if (ch == 0x0C)
	{
		// form-feed
		memset (state->display, ' ', sizeof state->display);
		state->xPos = 0;
		state->yPos = 0;
		return;
	}
	if (ch >= 127 || !isprint (ch) || (isspace (ch) && ch != ' '))
	{
		PIO_LOG_ERROR("Unexpected display character %02x", ch);
		ch = '?';
	}
	uint x = state->xPos;
	uint y = state->yPos;
	state->display[y][x] = ch;
	x++;
	if (x >= DisplayChars)
	{
		x = 0;
		y++;
		if (y >= DisplayLines)
			y = 0;
	}
	state->xPos = x;
	state->yPos = y;
}


static void eraseToLineEnd (State* state)
{
	PIO_LOG_TRACE("Erase to line end");
	uint x = state->xPos;
	uint y = state->yPos;
	uint len = DisplayChars - x;
	memset (state->display[y] + x, ' ', len);
}


static void readTelescope (State* state)
{
	byte buf[400];
	ssize_t count = pixi_read (state->serialFd, buf, sizeof (buf));
	PIO_LOG_TRACE("Read %zd bytes from serial input", count);
	bool displayUpdate = false;
	for (ssize_t i = 0; i < count; i++)
	{
		byte ch = buf[i];
		int newState = Ready;
		switch (state->cmdState)
		{
		case Ready:
			if (ch == 0x1B)
				newState = Escape;
			else
			{
				writeDisplayChar (state, ch);
				displayUpdate = true;
			}
			break;

		case Escape:
			switch (ch)
			{
			case 0x07: newState = Beep; break;
			case 0x13: newState = Sleep1; break;
			case 0x17: PIO_LOG_INFO("Waking up"); state->asleep = false; break;
			case 'C' : PIO_LOG_INFO("Cursor on"); break;
			case 'c' : PIO_LOG_INFO("Cursor off"); break;
			case 'E' : eraseToLineEnd (state); displayUpdate = true; break;
			case 'G' : newState = GotoX; break;
			case 'L' : newState = LcdBright; break;
			case 'l' : newState = KeyBright; break;
			case 'M' : PIO_LOG_INFO("Reading LED on"); break;
			case 'm' : PIO_LOG_INFO("Reading LED off"); break;
			case 'O' : newState = LcdContrast; break;
			default :
				PIO_LOG_ERROR("Unexpected escape byte %2X", ch);
			}
			break;

		case Sleep1:
		case Sleep2:
			if (ch == 0x13)
				newState = state->cmdState + 1;
			else
				PIO_LOG_ERROR("Unexpected sleep byte %2X", ch);
			break;
		case Sleep3:
			if (ch == 0)
			{
				PIO_LOG_INFO("Sleeping");
				state->asleep = true;
			}
			else
				PIO_LOG_ERROR("Unexpected sleep byte %2X", ch);
			break;

		case Beep:
			PIO_LOG_INFO("Beep duration %2X", ch);
			break;

		case GotoX:
			state->xPos = ch % DisplayChars;
			newState = GotoY;
			break;

		case GotoY:
			state->yPos = ch % DisplayLines;
			PIO_LOG_DEBUG("Moving to %u,%u", state->xPos, state->yPos);
			break;

		case LcdBright:
			PIO_LOG_INFO("LCD brightness %2X", ch);
			break;

		case KeyBright:
			PIO_LOG_INFO("Key-pad brightness %2X", ch);
			break;

		case LcdContrast:
			PIO_LOG_INFO("LCD contrast %2X", ch);
			break;

		default:
			PIO_LOG_FATAL("Logic error: unexpected state");
		}
		state->cmdState = newState;
	}
	if (displayUpdate)
	{
		// nul terminate lines
		for (int i = 0; i < DisplayLines; i++)
			state->display[i][DisplayChars] = 0;
		printf ("\n|%s|\n|%s|\n", state->display[0], state->display[1]);
	}
}


static void writeTelescope (State* state)
{
	PIO_LOG_TRACE("Serial ready for output");
	size_t writeFrom = BufferMask & (state->outputOffset - state->outputSize);
	size_t writeEnd  = writeFrom + state->outputSize;
	if (writeEnd > BufferLen)
		writeEnd = BufferLen;
	size_t size = writeEnd - writeFrom;
	ssize_t written = pixi_write (state->serialFd, state->outputBuf + writeFrom, size);
	if (written < 0)
	{
		PIO_ERROR(-written, "Error writing buffer to serial device");
		return;
	}
	if (pio_isLogLevelEnabled (LogLevelDebug))
	{
		char hexStr[1 + (BufferLen * 3)];
		char* ptr = hexStr;
		for (ssize_t i = 0; i < written; i++)
		{
			sprintf (ptr, "%02x ", state->outputBuf[writeFrom + i]);
			ptr += 3;
		}
		PIO_LOG_DEBUG("Wrote %zd bytes to serial: %s", written, hexStr);
	}
	state->outputSize -= written;
	if ((size_t) written == size && state->outputSize > 0)
		writeTelescope (state); // recurse just the once
}


static void appendBuffer (State* state, const char* buffer, size_t length)
{
	if (length + state->outputSize > BufferLen)
	{
		PIO_LOG_ERROR("Output buffer overflow");
		return;
	}
	size_t off = state->outputOffset;
	for (size_t i = 0; i < length; i++, off++)
		state->outputBuf[off & BufferMask] = buffer[i];
	state->outputOffset = off & BufferMask;
	state->outputSize  += length;
	PIO_LOG_DEBUG("Added %zu bytes to buffer, total now %zu", length, state->outputSize);
}


static void readInput (State* state)
{
	PIO_LOG_TRACE("Keyboard input");
	char buf[100];
	ssize_t count = pixi_read (state->inputFd, buf, sizeof (buf));
	PIO_LOG_TRACE("Keyboard input byte count = %zd", count);
/*
	for (int i = 0; i < count; i++)
		printf ("%2x ", (unsigned int) buf[i]);
	if (count > 0)
	{
		printf ("\n");
		fflush (stdout);
	}
*/
	// Quick and dirty: handle only isolated characters and ignore escapes, etc
	if (count == 1)
	{
		char out[2];
		char ch = toupper (buf[0]);
		switch (ch)
		{
		case 'M':
		case 'R':
		case 'G':
		case 'N':
		case 'S':
		case 'E':
		case 'W':
		case 'D':
		case 'L':
			out[0] = ch;
			out[1] = ch + 0x20;
			appendBuffer (state, out, 2);
			break;

		case '0':
		case '1':
		case '2':
		case '3':
		case '4':
		case '5':
		case '6':
		case '7':
		case '8':
		case '9':
			out[0] = ch;
			out[1] = ch + 0x80;
			appendBuffer (state, out, 2);
			break;
		}
	}
}


static void readKeypad (State* state)
{
	LIBPIXI_UNUSED(state);
	// TODO
}


static int runRemote (State* state)
{
	int result = setupSerial (state->serialFd);
	if (result < 0)
		PIO_LOG_WARN("Failed to configure serial device, but continuing anyway");

	const int waitMs = 300;
	const int count = 2;
	struct pollfd polls[count];

	polls[0].fd      = state->inputFd;
	polls[1].fd      = state->serialFd;

	do
	{
		polls[0].events  = POLLIN;
		polls[1].events  = POLLIN;
		if (!state->asleep && state->outputSize > 0)
			polls[1].events |= POLLOUT;
		polls[0].revents = 0;
		polls[1].revents = 0;
		result = poll (polls, count, waitMs);
//		PIO_LOG_TRACE("poll result = %d", result);
		if (result > 0)
		{
			if (polls[0].revents & POLLIN)
				readInput (state);
			if (polls[1].revents & POLLIN)
				readTelescope (state);
		}
		readKeypad (state);
		if (polls[1].revents & POLLOUT)
			writeTelescope (state);
	} while (result >= 0 || errno == EINTR);

	return 0;
}

static int remoteFn (uint argc, char*const*const argv)
{
	if (argc != 2)
	{
		PIO_LOG_ERROR ("usage: %s SERIAL-DEVICE", argv[0]);
		return -EINVAL;
	}
	const char* device = argv[1];
	int serialFd = pixi_open (device, O_RDWR | O_NONBLOCK, 0);
	if (serialFd < 0)
	{
		PIO_ERROR(-serialFd, "Failed to open serial device");
		return serialFd;
	}

	State state;
	memset (&state, 0, sizeof state);
	memset (state.display, ' ', sizeof state.display);
	state.inputFd = STDIN_FILENO;
	state.serialFd = serialFd;

	pixi_ttyInputRaw (state.inputFd);
	int result = runRemote (&state);
	pixi_ttyInputNormal (state.inputFd);

	pixi_close (serialFd);
	return result;
}

static Command remoteCmd =
{
	.name        = "telescope-remote",
	.description = "Operate as a remote control",
	.function    = remoteFn
};

static const Command* commands[] =
{
	&remoteCmd,
};

static CommandGroup telescopeGroup =
{
	.name      = "telescope",
	.count     = ARRAY_COUNT(commands),
	.commands  = commands,
	.nextGroup = NULL
};

static void PIO_CONSTRUCTOR (10003) initGroup (void)
{
	addCommandGroup (&telescopeGroup);
}
