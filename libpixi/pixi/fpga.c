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

#include <libpixi/pixi/fpga.h>
#include <libpixi/pixi/registers.h>
#include <libpixi/pixi/spi.h>
#include <libpixi/pi/gpio.h>
#include <libpixi/util/log.h>
#include <string.h>
#include <stdio.h>

#define LOW 0
#define HIGH 1


static void pinMode (int pin, Direction direction)
{
	pixi_piGpioChipSetPinMode(pin, direction);
}

static void digitalWrite (int pin, int value)
{
	pixi_piGpioChipWritePin(pin, value);
}

static int digitalRead (int pin)
{
	return pixi_piGpioChipReadPin(pin);
}

int64 pixi_fpgaGetVersion (void)
{
	int h = pixi_registerRead (Pixi_FPGA_build_time2);
	if (h < 0)
		return h;
	int m = pixi_registerRead (Pixi_FPGA_build_time1);
	if (m < 0)
		return m;
	int l = pixi_registerRead (Pixi_FPGA_build_time0);
	if (l < 0)
		return l;
	LIBPIXI_LOG_DEBUG("Got PiXi FPGA version %04x,%04x,%04x", h, m, l);

	return
		((uint64) h << 32) |
		((uint64) m << 16) |
		((uint64) l);
}

static int versionPart (const char* text, uint offset)
{
	int high = text[offset  ] - '0';
	int low  = text[offset+1] - '0';
	return (high * 10) + low;
}

int64 pixi_fpgaVersionToTime (int64 version)
{
	if (version <= 0)
	{
		LIBPIXI_LOG_DEBUG("pixi_fpgaVersionToTime invalid version %012llx", (ulonglong) version);
		return -EINVAL;
	}
	char buf[80];
	int chars = snprintf (buf, sizeof (buf), "%012llx", (ulonglong) version);
	if (chars != 12)
	{
		LIBPIXI_LOG_DEBUG("pixi_fpgaVersionToTime invalid version [%s]", buf);
		return -EINVAL;
	}

	struct tm tm;
	memset (&tm, 0, sizeof (tm));
	tm.tm_year = versionPart (buf, 10) + 100;
	tm.tm_mon  = versionPart (buf,  8) - 1;
	tm.tm_mday = versionPart (buf,  6);
	tm.tm_hour = versionPart (buf,  0);
	tm.tm_min  = versionPart (buf,  2);
	tm.tm_sec  = versionPart (buf,  4);
	time_t time = mktime (&tm);
	if (time < 0)
		return -errno;
	return time;
}

int pixi_fpgaLoadBuffer (const Buffer* _buffer)
{
	int result = pixi_piGpioMapRegisters();
	if (result < 0)
	{
		LIBPIXI_LOG_ERROR("Unable to map GPIO registers");
		return result;
	}

	const int PROG_PIN = pixi_piGpioMapWiringPiToChip (6);
	const int INIT_PIN = pixi_piGpioMapWiringPiToChip (2);
	const int CCLK_PIN = pixi_piGpioMapWiringPiToChip (0);
	const int DATA_PIN = pixi_piGpioMapWiringPiToChip (1);

	LIBPIXI_LOG_INFO("Setting pin I/O Direction...");
	pinMode(PROG_PIN, DirectionOut);
	pinMode(INIT_PIN, DirectionIn);
	pinMode(CCLK_PIN, DirectionOut);
	pinMode(DATA_PIN, DirectionOut);

//	int demo_build = spi_single_read(0, 0xff); // Check if a demo build is currently active in the FPGA

	LIBPIXI_LOG_INFO("Setting PROG low...");
	digitalWrite(PROG_PIN, LOW);

	// ***** Hold PROG low line for a short while *****
	usleep(1000);

	LIBPIXI_LOG_INFO("Setting PROG high...");
	digitalWrite(PROG_PIN, HIGH);

	LIBPIXI_LOG_INFO("Wait for INIT...");
	int timeout = 100;
	while ((timeout > 1) && (!(digitalRead(INIT_PIN) == HIGH)))
	{
		usleep(1000);
		timeout--;
	}

	if (digitalRead(INIT_PIN) == HIGH)
		LIBPIXI_LOG_INFO("Ready to program PiXi...");
	else
	{
		LIBPIXI_LOG_ERROR("INIT did not go high!");
		return -EINVAL; // ??
	}

	uint byte_counter = 0;
	int percent_complete = 0;

	usleep(1000000);

	const char* buffer = _buffer->memory;
	const size_t bytes_read = _buffer->size;
	const size_t bytes_readDIV10 = bytes_read / 10;

	// ***** Download to FPGA *****
	const char* percentFormat = "FPGA load %3d%% complete...";
	LIBPIXI_LOG_INFO(percentFormat, percent_complete);
	for (size_t i = 0; i < bytes_read; i++)
	{
		uchar byte = *(buffer + i);
		uchar shift_byte = byte;
//		uint repeat = 1;

//		while (repeat--)
		{
			for (uint j = 0; j < 8; j++)       //data goes out serially
			{
				// Set CCLK = '0'
				digitalWrite(CCLK_PIN, LOW);

				if (!(shift_byte & 0x80))
					digitalWrite(DATA_PIN, LOW);
				else
					digitalWrite(DATA_PIN, HIGH);

				// Set CCLK = '1', PROG = '1', DIN = data
				digitalWrite(CCLK_PIN, HIGH);

				shift_byte = shift_byte << 1;
			}
			byte_counter++;
		}

		if (byte_counter >= bytes_readDIV10)
		{
			percent_complete += 10;
			LIBPIXI_LOG_INFO(percentFormat, percent_complete);
			byte_counter -= bytes_readDIV10;
		}
	}

	// Need to contine clocking CCLK for a little while after download...
	for (uint j = 0; j < 8; j++)
	{
		digitalWrite(CCLK_PIN, LOW);
		digitalWrite(CCLK_PIN, HIGH);
	}

	usleep(100000);

	return 0;
}

int pixi_fpgaLoadFile (const char* filename)
{
	LIBPIXI_PRECONDITION_NOT_NULL(filename);

	Buffer buffer = BufferInit;
	int result = pixi_fileLoadContents (filename, &buffer);
	if (result < 0)
	{
		LIBPIXI_ERROR(-result, "Could not load file [%s]", filename);
		return result;
	}
	if (buffer.size > 0) // TODO: check for some larger minimum size?
	{
		LIBPIXI_LOG_DEBUG("Loaded FPGA image [%s] size=%zu", filename, buffer.size);
		result = pixi_fpgaLoadBuffer (&buffer);
		if (result < 0)
			LIBPIXI_ERROR(-result, "Could not load FPGA taken from file [%s]", filename);
	}
	else
	{
		LIBPIXI_LOG_ERROR("FPGA image [%s] is too small: size=%zu", filename, buffer.size);
		result = -ERANGE; // ??
	}
	free (buffer.memory);

	return result;
}

int64 pixi_fpgaGetBuildTime (void)
{
	int64 version = pixi_fpgaGetVersion();
	if (version < 0)
		return version;
	int64 build = pixi_fpgaVersionToTime (version);
	return build;
}

int64 pixi_fpgaGetId (void)
{
	int id0 = pixi_registerRead (Pixi_FPGA_dna0);
	if (id0 < 0)
		return id0;
	int id1 = pixi_registerRead (Pixi_FPGA_dna1);
	if (id1 < 1)
		return id1;
	int id2 = pixi_registerRead (Pixi_FPGA_dna2);
	if (id2 < 2)
		return id2;
	int id3 = pixi_registerRead (Pixi_FPGA_dna3);
	if (id3 < 3)
		return id3;

	LIBPIXI_LOG_DEBUG("Got PiXi FPGA DNA ID %04x,%04x,%04x,%04x", id0, id1, id2, id3);

	return
		((uint64) id3 << 48) |
		((uint64) id2 << 32) |
		((uint64) id1 << 16) |
		((uint64) id0);
}
