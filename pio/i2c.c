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

#include <libpixi/pi/i2c.h>
#include <libpixi/util/file.h>
#include <libpixi/util/string.h>
#include <stdio.h>
#include "common.h"
#include "log.h"


static int i2cTransfer (const Command* command, uint argc, char* argv[])
{
	if (argc < 4)
		return commandUsageError (command);

	const uint dataOffset = 4;
	uint channel   = pixi_parseLong (argv[1]);
	uint address   = pixi_parseLong (argv[2]);
	uint rxSize    = pixi_parseLong (argv[3]);
	uint txSize    = argc - dataOffset;
	if (rxSize > 2048)
	{
		PIO_LOG_ERROR("Transfer size of %u is too large", rxSize);
		return -EINVAL;
	}
	if (txSize > 2048)
	{
		PIO_LOG_ERROR("Transfer size of %u is too large", txSize);
		return -EINVAL;
	}

	I2cDevice dev = I2C_DEVICE_INIT;
	int result = pixi_i2cOpen2 (channel, address, &dev);
	if (result < 0)
	{
		PIO_ERROR(-result, "Couldn't open flash I2C channel");
		return result;
	}
	uint hexSize = 1 + (rxSize * 3);
	char* hex = malloc (hexSize);
	uint8* tx = malloc (txSize);
	uint8* rx = malloc (rxSize);
	if (hex && tx && rx)
	{
		for (uint i = 0; i < txSize; i++)
			tx[i] = pixi_parseLong (argv[dataOffset + i]);

		result = pixi_i2cReadWrite (&dev, tx, txSize, rx, rxSize);
		if (result >= 0)
		{
			pixi_hexEncode (rx, rxSize, hex, hexSize, ' ', "");
			printf ("result: [%s]\n", hex);
			result = 0;
		}
		else
			PIO_ERROR(-result, "I2C read/write failed");
	}
	else
	{
		PIO_LOG_FATAL("Failed to allocate buffers of size %u and %u", rxSize, txSize);
		result = -ENOMEM;
	}
	pixi_i2cClose (&dev);

	free (tx);
	free (rx);
	free (hex);
	return result;
}
static Command i2cTransferCmd =
{
	.name        = "i2c-transfer",
	.description = "Perform an I2C transfer",
	.usage       = "usage: %s CHANNEL ADDRESS RX-SIZE TX-BYTES...",
	.function    = i2cTransfer
};

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
	uint hexSize = 1 + (size * 3);
	char* hex = malloc (hexSize);
	uint8* rx = malloc (size);
	if (hex && rx)
	{
		result = pixi_read (i2c, rx, size);
		if (result < 0)
			PIO_ERROR(-result, "i2c read failed");
		else
		{
			size = result;
			result = 0;
			PIO_LOG_INFO("Read %u bytes", size);

			pixi_hexEncode (rx, size, hex, hexSize, ' ', "");
			printf ("result: [%s]\n", hex);
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
	uint8* tx = malloc (size);
	if (tx)
	{
		for (uint i = 0; i < size; i++)
			tx[i] = pixi_parseLong (argv[dataOffset + i]);

		PIO_LOG_INFO("Writing %u bytes", size);
		result = pixi_write (i2c, tx, size);
		if (result < 0)
			PIO_ERROR(-result, "i2c write failed");
		else if (result != (int) size)
		{
			PIO_LOG_ERROR("Failed to write all bytes [%u]: wrote %u", size, result);
			result = -EIO;
		}
		else
		{
			PIO_LOG_INFO("Wrote %u bytes", result);
			result = 0;
		}
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
	&i2cTransferCmd,
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

static void PIO_CONSTRUCTOR (1050) initGroup (void)
{
	addCommandGroup (&pixiI2cGroup);
}
