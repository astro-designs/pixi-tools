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
#include <libpixi/pixi/lcd.h>
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

	// Enable software flow control, otherwise we'll never receive
	// the 0x13/DC3 bytes in the sleep message
	term.c_iflag &= ~(IXON | IXOFF);

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
	PollInterval = 40, // milliseconds

	BufferLen  = 256,
	BufferMask = BufferLen - 1,
	DisplayChars = 40, // per line
	DisplayLines = 2,

	KeyPadRegister = 0x33,
	KeyMask        = 0x07F,
	KeyStateMask   = 0xF00,
	KeyBufferEmpty = 0x100,
	KeyBufferFull  = 0x200,
	KeyPressed     = 0x400,
	KeyReleased    = 0x800,

	Rotary1Register = 0x61,
	Rotary2Register = 0x60,
	RotaryThreshold = 5,

	KeyCount = 24
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
	LcdDevice device;
	bool      usePixi;

	uint8     rotary1;
	uint8     rotary2;

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
	uint      numKeys; // bitmap
} State;


static const char keyUp[]    = {0x1b, 0x5b, 0x41, 0};
static const char keyDown[]  = {0x1b, 0x5b, 0x42, 0};
static const char keyRight[] = {0x1b, 0x5b, 0x43, 0};
static const char keyLeft[]  = {0x1b, 0x5b, 0x44, 0};


static void clearDisplay (State* state)
{
	PIO_LOG_INFO("Clearing display");
	memset (state->display, ' ', sizeof state->display);
	state->xPos = 0;
	state->yPos = 0;

	if (state->usePixi)
	{
		// pixi_lcdClear seems to cause problems
		pixi_lcdSetCursorPos (&state->device, 0, 0);
		state->display[0][DisplayChars] = 0;
		pixi_lcdWriteStr (&state->device, state->display[0]);
		pixi_lcdWriteStr (&state->device, state->display[0]);
	}
}


static void initState (State* state)
{
	memset (state, 0, sizeof (State));
	initLcdDevice (&state->device);
}


static void writeDisplayChar (State* state, byte ch)
{
	if (ch == 0x0C)
	{
		// form-feed
		clearDisplay (state);
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

	if (state->usePixi)
	{
		char str[2] = {ch, 0};
		if (ch == 0)
			str[0] = ' ';
		pixi_lcdWriteStr (&state->device, str);
	}

}


static void eraseToLineEnd (State* state)
{
	PIO_LOG_TRACE("Erase to line end");
	uint x = state->xPos;
	uint y = state->yPos;
	uint len = DisplayChars - x;
	memset (state->display[y] + x, ' ', len);
	if (state->usePixi)
	{
		state->display[y][DisplayChars] = 0;
		pixi_lcdWriteStr (&state->device, state->display[y] + x);
		pixi_lcdSetCursorPos (&state->device, x, y);
	}
}


static void sendGotoPos (State* state)
{
	if (!state->usePixi)
		return;
	pixi_lcdSetCursorPos (&state->device, state->xPos, state->yPos);
}

static const char printableChars[] = " 0123456789ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz,./<>?;:'@#~][}{=-+_`!\"$^&*()";

static void readTelescope (State* state)
{
	byte buf[400];
	ssize_t count = pixi_read (state->serialFd, buf, sizeof (buf));
	if (pio_isLogLevelEnabled(LogLevelDebug))
	{
		char printable[1+(sizeof(buf)*3)];
		pixi_hexEncode (buf, count, printable, sizeof (printable), '%', printableChars);
		PIO_LOG_DEBUG("Received: as characters : [%s]", printable);
		pixi_hexEncode (buf, count, printable, sizeof (printable), ' ', "");
		PIO_LOG_DEBUG("Received: as hexadecimal: [%s]", printable);
	}
	bool displayUpdate = false;
	for (ssize_t i = 0; i < count; i++)
	{
		byte ch = buf[i];
		PIO_LOG_TRACE("handle byte: %2x", ch);
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
			sendGotoPos (state);
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
		PIO_LOG_INFO("Display: |%s|", state->display[0]);
		PIO_LOG_INFO("         |%s|", state->display[1]);
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


static void appendChar (State* state, char key)
{
	uchar keyVal = key;
	if (key >= '0' && key <= '9')
	{
		int value = key - '0';
		state->numKeys |= 1 << value;
		PIO_LOG_DEBUG("numKeys: %x", state->numKeys);
		// Three fingered salute: hold, 1, 6, 7 to halt the machine
		if (state->numKeys == ((1<<1) | (1<<6) | (1<<7)))
		{
			PIO_LOG_WARN("Halting");
			system ("halt");
			if (state->usePixi)
			{
				pixi_lcdSetCursorPos (&state->device, 0, 1);
				pixi_lcdWriteStr (&state->device, "Halting...");
			}
		}
	}
	else if (keyVal >= ('0' + 0x80) && keyVal <= ('9' + 0x80))
	{
		// Four fingered salute: hold, 1, 3, 7, 9 to halt the machine
		int value = (keyVal - 0x80) - '0';
		state->numKeys &= ~(1 << value);
		PIO_LOG_DEBUG("numKeys: %x", state->numKeys);
	}
	appendBuffer (state, &key, 1);
}


static void keyPressRelease (State* state, char key)
{
	char buf[2] = {key, key};
	if (key >= '0' && key <= '9')
		buf[1] += 0x80;
	else
		buf[1] += 0x20;
	appendBuffer (state, buf, sizeof (buf));
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
		case 'U':
		case 'D':
		case 'L':
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
			keyPressRelease (state, ch);
		}
	}
}


static void readKeypad (State* state)
{
	if (!state->usePixi)
		return;

	for (int i = 0; i < 20; i++) // avoid infinite loops
	{
		int reg = pixi_registerRead (&state->device.spi, KeyPadRegister);
		if (reg & KeyBufferEmpty)
			return;

		PIO_LOG_DEBUG("Handling key register %03x", reg);
		if (!(reg & (KeyReleased | KeyPressed)))
			continue;

		uint key = reg & KeyMask;
		if (reg & KeyReleased)
		{
			PIO_LOG_INFO("Key release: '%c'", key);
			if (key >= '0' && key <= '9')
				key += 0x80;
			else
				key += 0x20;
		}
		else
			PIO_LOG_INFO("Key press: '%c'", key);

		char ch = key;
		appendChar (state, ch);
	}
	PIO_LOG_FATAL("Excessive key events!");
}


static void readRotary (State* state)
{
	if (!state->usePixi)
		return;

	uint8 rotary1 = pixi_registerRead (&state->device.spi, Rotary1Register);
	uint8 rotary2 = pixi_registerRead (&state->device.spi, Rotary2Register);
	if (rotary1 != state->rotary1)
	{
		int8 delta = state->rotary1 - rotary1;
		if (delta < - RotaryThreshold)
		{
			PIO_LOG_DEBUG("Rotary 1: %04x, change %d", rotary1, delta);
			state->rotary1 = rotary1;
			PIO_LOG_INFO("Scroll up");
			keyPressRelease (state, 'U');
		}
		else if (delta > RotaryThreshold)
		{
			PIO_LOG_DEBUG("Rotary 1: %04x, change %d", rotary1, delta);
			state->rotary1 = rotary1;
			PIO_LOG_INFO("Scroll down");
			keyPressRelease (state, 'D');
		}
	}
	if (rotary2 != state->rotary2)
	{
		PIO_LOG_DEBUG("Rotary 2: %04x", rotary2);
		state->rotary2 = rotary2;
	}
}


static int runRemote (State* state)
{
	int result = setupSerial (state->serialFd);
	if (result < 0)
		PIO_LOG_WARN("Failed to configure serial device, but continuing anyway");

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
		result = poll (polls, count, PollInterval);
//		PIO_LOG_TRACE("poll result = %d", result);
		if (result > 0)
		{
			if (polls[0].revents & POLLIN)
				readInput (state);
			if (polls[1].revents & POLLIN)
				readTelescope (state);
		}
		readKeypad (state);
		readRotary (state);
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
	State state;
	initState (&state);

	const char* device = argv[1];
	int serialFd = pixi_open (device, O_RDWR | O_NONBLOCK, 0);
	if (serialFd < 0)
	{
		PIO_ERROR(-serialFd, "Failed to open serial device");
		return serialFd;
	}
	int result = pixi_lcdOpen (&state.device);
	if (result < 0)
	{
		state.usePixi = false;
		PIO_ERROR(-result, "Failed to open PiXi, but continuing anyway");
	}
	else
	{
		state.usePixi = true;
		state.rotary1 = pixi_registerRead (&state.device.spi, Rotary1Register);
		state.rotary2 = pixi_registerRead (&state.device.spi, Rotary2Register);
	}
	clearDisplay (&state);

	state.inputFd = STDIN_FILENO;
	state.serialFd = serialFd;

	pixi_ttyInputRaw (state.inputFd);
	result = runRemote (&state);
	pixi_ttyInputNormal (state.inputFd);

	pixi_close (serialFd);
	if (state.usePixi)
		pixi_lcdClose (&state.device);
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
