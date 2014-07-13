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

#include <libpixi/pixi/simple.h>
#include <libpixi/pi/spi.h>
#include <libpixi/util/string.h>
#include "Command.h"
#include "log.h"
#include <stdio.h>

static int flashOpen (SpiDevice* device)
{
	int result = pixi_spiOpen (0, PixiSpiSpeed, device);
	if (result < 0)
		PIO_ERROR(-result, "Couldn't open flash SPI channel");
	return result;
}

static int flashRdpResFn (const Command* command, uint argc, char* argv[])
{
	LIBPIXI_UNUSED(argv);

	if (argc != 1)
		return commandUsageError (command);

	SpiDevice dev = SPI_DEVICE_INIT;
	int result = flashOpen (&dev);
	if (result < 0)
		return result;

	uint8 tx[5] = {
		0xAB,
		0,
		0,
		0,
		0
	};
	uint8 rx[5];
	result = pixi_spiReadWrite (&dev, tx, rx, sizeof (tx));
	pixi_spiClose (&dev);
	if (result < 0)
	{
		PIO_ERROR(-result, "SPI read/write failed");
		return result;
	}
	char hex[40];
	pixi_hexEncode (rx, sizeof (rx), hex, sizeof (hex), ' ', "");
	printf ("Result=[%s]\n", hex);
	return 0;
}
static Command flashRdpResCmd =
{
	.name        = "flash-rdp-res",
	.description = "Release flash from deep power down and read electronic signature",
	.usage       = "usage: %s",
	.function    = flashRdpResFn
};
static int flashReadIdFn (const Command* command, uint argc, char* argv[])
{
	LIBPIXI_UNUSED(argv);

	if (argc != 1)
		return commandUsageError (command);

	SpiDevice dev = SPI_DEVICE_INIT;
	int result = flashOpen (&dev);
	if (result < 0)
		return result;

	uint8 tx[4] = {
		0x9F,
		0,
		0,
		0
	};
	uint8 rx[4];
	result = pixi_spiReadWrite (&dev, tx, rx, sizeof (tx));
	pixi_spiClose (&dev);
	if (result < 0)
	{
		PIO_ERROR(-result, "SPI read/write failed");
		return result;
	}
	uint id = (rx[0] << 24) | (rx[1] << 16) | (rx[2] << 8) | rx[3];
	printf ("0x%08X\n", id);
	return 0;
}
static Command flashReadIdCmd =
{
	.name        = "flash-read-id",
	.description = "read flash manufacturer ID",
	.usage       = "usage: %s",
	.function    = flashReadIdFn
};

static int flashReadStatusFn (const Command* command, uint argc, char* argv[])
{
	LIBPIXI_UNUSED(argv);

	if (argc != 1)
		return commandUsageError (command);

	SpiDevice dev = SPI_DEVICE_INIT;
	int result = flashOpen (&dev);
	if (result < 0)
		return result;

	uint8 tx[4] = {
		0x05,
		0,
		0,
		0
	};
	uint8 rx[4];
	result = pixi_spiReadWrite (&dev, tx, rx, sizeof (tx));
	pixi_spiClose (&dev);
	if (result < 0)
	{
		PIO_ERROR(-result, "SPI read/write failed");
		return result;
	}
	uint id = (rx[0] << 24) | (rx[1] << 16) | (rx[2] << 8) | rx[3];
	printf ("0x%08X\n", id);
	return 0;
}
static Command flashReadStatusCmd =
{
	.name        = "flash-read-status",
	.description = "read flash status register",
	.usage       = "usage: %s",
	.function    = flashReadStatusFn
};

static int flashReadMemoryFn (const Command* command, uint argc, char* argv[])
{
	if (argc != 3)
		return commandUsageError (command);

	uint address = pixi_parseLong (argv[1]);
	uint length  = pixi_parseLong (argv[2]);
	if (length > 0x1000000)
	{
		PIO_LOG_ERROR("Length of %u is too big", length);
		return -EINVAL;
	}

	SpiDevice dev = SPI_DEVICE_INIT;
	int result = flashOpen (&dev);
	if (result < 0)
		return result;

	uint header = 4;
	uint size = header + length;
	uint8* tx = 0;
	uint8* rx = 0;
	char* hex = 0;
	tx = malloc (size);
	rx = malloc (size);
	if (tx && rx)
	{
		tx[0] = 0x03;
		tx[1] = address << 16;
		tx[2] = address <<  8;
		tx[3] = address <<  0;

		result = pixi_spiReadWrite (&dev, tx, rx, size);
		pixi_spiClose (&dev);
		if (result >= 0)
		{
			uint hexSize = 1 + (size * 3);
			hex = malloc (hexSize);
			if (hex)
			{
				pixi_hexEncode (rx, size, hex, hexSize, ' ', "");
				printf ("memory: [%s]\n", hex);
			}
			else
			{
				PIO_ERROR(-result, "Read/write succeeded, but could not allocate print buffer");
				result = -ENOMEM;
			}
		}
		else
		{
			PIO_ERROR(-result, "SPI read/write failed");
			return result;
		}
		return 0;
	}
	else
	{
		PIO_LOG_FATAL("Failed to allocate buffers of size %u", size);
		result = -ENOMEM;
	}

	free (tx);
	free (rx);
	free (hex);
	return result;
}
static Command flashReadMemoryCmd =
{
	.name        = "flash-read",
	.description = "read flash memory",
	.usage       = "usage: %s ADDRESS LENGTH",
	.function    = flashReadMemoryFn
};

static const Command* commands[] =
{
	&flashRdpResCmd,
	&flashReadIdCmd,
	&flashReadStatusCmd,
	&flashReadMemoryCmd,
};


static CommandGroup pixiFlashGroup =
{
	.name      = "pixi-flash",
	.count     = ARRAY_COUNT(commands),
	.commands  = commands,
	.nextGroup = NULL
};

static void PIO_CONSTRUCTOR (103) initGroup (void)
{
	addCommandGroup (&pixiFlashGroup);
}
