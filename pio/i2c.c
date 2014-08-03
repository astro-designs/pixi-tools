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

#include <libpixi/util/file.h>
#include <libpixi/util/string.h>
#include "Command.h"
#include "log.h"
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/ioctl.h>
#include <unistd.h>
#include <linux/i2c-dev.h>

const char spiDevice[] = "/dev/i2c-%d";

int pixi_i2cOpen (uint channel, uint address);
int pixi_i2cOpen (uint channel, uint address)
{
	LIBPIXI_PRECONDITION(channel < 16);
	LIBPIXI_PRECONDITION(address < 1024);

	char filename[40];
	snprintf (filename, sizeof (filename), spiDevice, channel);
	int fd = pixi_open (filename, O_RDWR, 0);
	if (fd < 0)
	{
		LIBPIXI_ERROR_DEBUG(-fd, "failed to open i2c device [%s] failed", filename);
		return fd;
	}

	LIBPIXI_LOG_DEBUG("Opened i2c name=%s fd=%d", filename, fd);
	int result = ioctl (fd, I2C_SLAVE, address);
	if (result < 0)
	{
		int err = errno;
		LIBPIXI_ERROR(err, "failed to set i2c address to %u", address);
		pixi_close (fd);
		return -err;
	}

	return fd;
}


static int i2cRead (const Command* command, uint argc, char* argv[])
{
	if (argc != 4)
		return commandUsageError (command);

	uint channel = pixi_parseLong (argv[1]);
	uint address = pixi_parseLong (argv[2]);
	uint size    = pixi_parseLong (argv[3]);
	if (size > 0x100000)
	{
		PIO_LOG_ERROR("Transfer size of %u is too large", size);
		return -EINVAL;
	}

	int i2c = pixi_i2cOpen (channel, address);
	if (i2c < 0)
	{
		PIO_ERROR(-i2c, "Couldn't open i2c device");
		return i2c;
	}

	int result = 0;
	uint8* rx = 0;
	char* hex = 0;
	rx = malloc (size);
	if (rx)
	{
		result = pixi_read (i2c, rx, size);
		if (result < 0)
			PIO_ERROR(-result, "i2c read failed");
		else
		{
			size = result;
			result = 0;
			PIO_LOG_INFO("Read %u bytes", size);

			uint hexSize = 1 + (size * 3);
			hex = malloc (hexSize);
			if (hex)
			{
				pixi_hexEncode (rx, size, hex, hexSize, ' ', "");
				printf ("result: [%s]\n", hex);
			}
			else
			{
				PIO_ERROR(-result, "Read succeeded, but could not allocate print buffer");
				result = -ENOMEM;
			}

		}
	}
	else
	{
		PIO_LOG_FATAL("Failed to allocate buffer of size %u", size);
		result = -ENOMEM;
	}
	pixi_close (i2c);

	free (rx);
	free (hex);
	return result;
}
static Command i2cReadCmd =
{
	.name        = "i2c-read",
	.description = "Perform an i2c read",
	.usage       = "usage: %s CHANNEL ADDRESS SIZE",
	.function    = i2cRead
};


static int i2cWrite (const Command* command, uint argc, char* argv[])
{
	if (argc < 4)
		return commandUsageError (command);

	const uint dataOffset = 3;
	uint channel = pixi_parseLong (argv[1]);
	uint address = pixi_parseLong (argv[2]);
	uint size    = argc - dataOffset;
	if (size > 4096)
	{
		PIO_LOG_ERROR("Transfer size of %u is too large", size);
		return -EINVAL;
	}

	int i2c = pixi_i2cOpen (channel, address);
	if (i2c < 0)
	{
		PIO_ERROR(-i2c, "Couldn't open i2c device");
		return i2c;
	}

	int result = 0;
	uint8* tx = 0;
	tx = malloc (size);
	if (tx)
	{
		for (uint i = 0; i < size; i++)
			tx[i] = pixi_parseLong (argv[dataOffset + i]);

		PIO_LOG_INFO("Writing %u bytes", size);
		result = pixi_write (i2c, tx, size);
		if (result < 0)
			PIO_ERROR(-result, "i2c write failed");
		else if (result != (int) size)
			PIO_LOG_ERROR("Failed to write all bytes [%u]: wrote %u", size, result);
		else
			PIO_LOG_INFO("Wrote %u bytes", result);
	}
	else
	{
		PIO_LOG_FATAL("Failed to allocate buffer of size %u", size);
		result = -ENOMEM;
	}
	pixi_close (i2c);

	free (tx);
	return result;
}
static Command i2cWriteCmd =
{
	.name        = "i2c-write",
	.description = "Perform an i2c write",
	.usage       = "usage: %s CHANNEL ADDRESS TX-BYTES...",
	.function    = i2cWrite
};

static const Command* commands[] =
{
	&i2cReadCmd,
	&i2cWriteCmd,
};


static CommandGroup pixiI2cGroup =
{
	.name      = "pixi-i2c",
	.count     = ARRAY_COUNT(commands),
	.commands  = commands,
	.nextGroup = NULL
};

static void PIO_CONSTRUCTOR (103) initGroup (void)
{
	addCommandGroup (&pixiI2cGroup);
}
