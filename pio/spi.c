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

#include <libpixi/pixi/registers.h>
#include <libpixi/pixi/spi.h>
#include <libpixi/util/string.h>
#include "Command.h"
#include "log.h"
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

static int spiSetGet (char*const*const argv, int mode)
{
	uint channel = pixi_parseLong (argv[1]);
	uint address = pixi_parseLong (argv[2]);
	uint data    = pixi_parseLong (argv[3]);

	SpiDevice dev = SpiDeviceInit;
	int result = pixi_spiOpen(channel, PixiSpiSpeed, &dev);
	if (result < 0)
	{
		PIO_ERROR(-result, "Failed to setup SPI device %u", channel);
		return result;
	}
	// TODO: belongs in libpixi
	uint8_t buffer[4] = {
		address,
		mode,
		(data & 0xFF00) >> 8,
		(data & 0x00FF)
	};
	result = pixi_spiReadWrite(&dev, buffer, buffer, sizeof (buffer));
	pixi_spiClose (&dev);
	if (result < 0)
		PIO_ERROR(-result, "SPI read-write failed");

	data = (buffer[2] << 8) | buffer[3];
	PIO_LOG_INFO("SPI returned: 0x%02x 0x%02x 0x%04x", buffer[0], buffer[1], data);
	return result;
}

static int spiSetFn (const Command* command, uint argc, char* argv[])
{
	if (argc != 4)
		return commandUsageError (command);

	return spiSetGet (argv, PixiSpiEnableWrite8); // FIXME: Write8?
}
static Command spiSetCmd =
{
	.name        = "spi-set",
	.description = "write a value to the pixi over spi",
	.usage       = "usage: %s CHANNEL ADDRESS VALUE",
	.function    = spiSetFn
};

static int spiGetFn (const Command* command, uint argc, char* argv[])
{
	if (argc != 4)
		return commandUsageError (command);

	return spiSetGet (argv, PixiSpiEnableRead16); // FIXME: Read16?
}
static Command spiGetCmd =
{
	.name        = "spi-get",
	.description = "read a value from the pixi over spi",
	.usage       = "usage: %s CHANNEL ADDRESS NULL-DATA",
	.function    = spiGetFn
};


static int monitorSpi (uint channel, uint address)
{
	SpiDevice dev = SpiDeviceInit;
	int result = pixi_spiOpen(channel, PixiSpiSpeed, &dev);
	if (result < 0)
	{
		PIO_ERROR(-result, "Failed to setup SPI device %u", channel);
		return result;
	}

	uint previous = -1;
	uint changes = 0;
	while (true)
	{
		uint data = 0;
		// TODO: belongs in libpixi
		uint8_t buffer[4] = {
			address,
			PixiSpiEnableRead16,
			(data & 0xFF00) >> 8,
			(data & 0x00FF)
		};
		result = pixi_spiReadWrite(&dev, buffer, buffer, sizeof (buffer));
		if (result < 0)
			break;

		data = (buffer[2] << 8) | buffer[3];
		if (previous != data)
		{
			changes++;
			previous = data;
			printf("\r%08u: 0x%04x", changes, data);
			fflush (stdout);
		}
		// Slow enough to glimpse each change
		usleep (100 * 1000);
	}
	printf("\n");

	pixi_spiClose (&dev);
	if (result < 0)
		PIO_ERROR(-result, "SPI read-write failed");

	return result;
}

static int spiMonitorFn (const Command* command, uint argc, char* argv[])
{
	if (argc != 3)
		return commandUsageError (command);

	uint channel = pixi_parseLong (argv[1]);
	uint address = pixi_parseLong (argv[2]);

	return monitorSpi (channel, address);
}
static Command spiMonitorCmd =
{
	.name        = "spi-monitor",
	.description = "monitor an SPI channel/address",
	.usage       = "usage: %s CHANNEL ADDRESS",
	.function    = spiMonitorFn
};


static int scanSpi (uint channel)
{
	SpiDevice dev = SpiDeviceInit;
	int result = pixi_spiOpen(channel, PixiSpiSpeed, &dev);
	if (result < 0)
	{
		PIO_ERROR(-result, "Failed to setup SPI device %u", channel);
		return result;
	}

	uint16 memory[256];
	memset (memory, 0, sizeof (memory));
	uint changes = 0;

	while (true)
	{
		for (uint addr = 0; addr < 256; addr++)
		{
			uint data = 0;
			// TODO: belongs in libpixi
			uint8_t buffer[4] = {
				addr,
				PixiSpiEnableRead16,
				(data & 0xFF00) >> 8,
				(data & 0x00FF)
			};
			result = pixi_spiReadWrite(&dev, buffer, buffer, sizeof (buffer));
			if (result < 0)
				break;

			data = (buffer[2] << 8) | buffer[3];
			if (memory[addr] != data)
			{
				changes++;
				memory[addr] = data;
				printf("%08u: %3u 0x%04x\n", changes, addr, data);
				fflush (stdout);
			}
		}
	}
	printf("\n");

	pixi_spiClose (&dev);
	if (result < 0)
		PIO_ERROR(-result, "SPI read-write failed");

	return result;
}

static int spiScanFn (const Command* command, uint argc, char* argv[])
{
	if (argc != 2)
		return commandUsageError (command);

	uint channel = pixi_parseLong (argv[1]);

	return scanSpi (channel);
}
static Command spiScanCmd =
{
	.name        = "spi-scan",
	.description = "continually scan an SPI channel (all addresses)",
	.usage       = "usage: %s CHANNEL",
	.function    = spiScanFn
};

static int monitorButtonsFn (const Command* command, uint argc, char* argv[])
{
	LIBPIXI_UNUSED(argv);
	if (argc != 1)
		return commandUsageError (command);

	return monitorSpi (0, Pixi_Switch_in);
}
static Command monitorButtonsCmd =
{
	.name        = "monitor-buttons",
	.description = "monitor the PiXi on-board buttons",
	.usage       = "usage: %s",
	.function    = monitorButtonsFn
};

static const Command* commands[] =
{
	&spiSetCmd,
	&spiGetCmd,
	&spiMonitorCmd,
	&spiScanCmd,
	&monitorButtonsCmd
};


static CommandGroup pixiSpiGroup =
{
	.name      = "pixi-spi",
	.count     = ARRAY_COUNT(commands),
	.commands  = commands,
	.nextGroup = NULL
};

static void PIO_CONSTRUCTOR (102) initGroup (void)
{
	addCommandGroup (&pixiSpiGroup);
}
