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

static int spiTransfer (const Command* command, uint argc, char* argv[])
{
	if (argc < 4)
		return commandUsageError (command);

	const uint dataOffset = 3;
	uint channel   = pixi_parseLong (argv[1]);
	uint frequency = pixi_parseLong (argv[2]);
	uint size      = argc - dataOffset;
	if (size > 4096)
	{
		PIO_LOG_ERROR("Transfer size of %u is too large", size);
		return -EINVAL;
	}

	SpiDevice dev = SPI_DEVICE_INIT;
	int result = pixi_spiOpen (channel, frequency, &dev);
	if (result < 0)
	{
		PIO_ERROR(-result, "Couldn't open flash SPI channel");
		return result;
	}
	uint hexSize = 1 + (size * 3);
	char* hex = malloc (hexSize);
	uint8* tx = malloc (size);
	uint8* rx = malloc (size);
	if (hex && tx && rx)
	{
		for (uint i = 0; i < size; i++)
			tx[i] = pixi_parseLong (argv[dataOffset + i]);

		result = pixi_spiReadWrite (&dev, tx, rx, size);
		if (result >= 0)
		{
			pixi_hexEncode (rx, size, hex, hexSize, ' ', "");
			printf ("result: [%s]\n", hex);
			result = 0;
		}
		else
			PIO_ERROR(-result, "SPI read/write failed");
	}
	else
	{
		PIO_LOG_FATAL("Failed to allocate buffers of size %u", size);
		result = -ENOMEM;
	}
	pixi_spiClose (&dev);

	free (tx);
	free (rx);
	free (hex);
	return result;
}
static Command spiTransferCmd =
{
	.name        = "spi-transfer",
	.description = "Perform an SPI transfer",
	.usage       = "usage: %s CHANNEL FREQUENCY TX-BYTES...",
	.function    = spiTransfer
};

static int spiSetGet (bool writeMode, uint channel, uint address, uint data)
{
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
		writeMode ? PixiSpiEnableWrite8 : PixiSpiEnableRead16, // FIXME: erm, check this
		(data & 0xFF00) >> 8,
		(data & 0x00FF)
	};
	result = pixi_spiReadWrite(&dev, buffer, buffer, sizeof (buffer));
	pixi_spiClose (&dev);
	if (result < 0)
		PIO_ERROR(-result, "SPI read-write failed");

	data = (buffer[2] << 8) | buffer[3];
	PIO_LOG_INFO("SPI returned: 0x%02x 0x%02x 0x%04x", buffer[0], buffer[1], data);
	if (!writeMode)
		printf ("%u\n", data);
	return result;
}

static int spiSetFn (const Command* command, uint argc, char* argv[])
{
	if (argc < 3 || argc > 4)
		return commandUsageError (command);

	uint channel = PixiSpiChannel;
	uint address = 0;
	uint data    = 0;
	int arg = 1;
	if (argc > 3)
		channel = pixi_parseLong (argv[arg++]);
	address = pixi_parseLong (argv[arg++]);
	data    = pixi_parseLong (argv[arg++]);

	return spiSetGet (true, channel, address, data);
}
static Command spiSetCmd =
{
	.name        = "spi-set",
	.description = "write a value to the pixi over spi",
	.usage       = "usage: %s [CHANNEL] ADDRESS VALUE",
	.function    = spiSetFn
};

static int spiGetFn (const Command* command, uint argc, char* argv[])
{
	if (argc < 2 || argc > 4)
		return commandUsageError (command);

	uint channel = PixiSpiChannel;
	uint address = 0;
	uint data    = 0;
	int arg = 1;
	if (argc > 2)
		channel = pixi_parseLong (argv[arg++]);
	address = pixi_parseLong (argv[arg++]);
	if (argc > 3)
		data    = pixi_parseLong (argv[arg++]);

	return spiSetGet (false, channel, address, data);
}
static Command spiGetCmd =
{
	.name        = "spi-get",
	.description = "read a value from the pixi over spi",
	.usage       = "usage: %s [CHANNEL] ADDRESS [NULL-DATA]",
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


static int scanSpi (uint channel, uint low, uint high, uint sleepUs)
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
		for (uint addr = low; addr <= high; addr++)
		{
			// TODO: belongs in libpixi
			uint8 txBuf[4] = {addr, PixiSpiEnableRead16, 0, 0};
			uint8 rxBuf[4] = {0,0,0,0};
			result = pixi_spiReadWrite(&dev, txBuf, rxBuf, sizeof (txBuf));
			if (result < 0)
				break;

			if (pio_isLogLevelEnabled (LogLevelDebug))
			{
				char txHex[16];
				char rxHex[16];
				pixi_hexEncode (txBuf, sizeof (txBuf), txHex, sizeof (txHex), ' ', "");
				pixi_hexEncode (rxBuf, sizeof (rxBuf), rxHex, sizeof (rxHex), ' ', "");
				PIO_LOG_DEBUG("SPI tx=[%s] rx=[%s]", txHex, rxHex);
			}
			uint data = (rxBuf[2] << 8) | rxBuf[3];
			if (memory[addr] != data)
			{
				changes++;
				memory[addr] = data;
				char time[26];
				pixi_formatCurTime (time, sizeof (time));
				printf("%s: 0x%02x 0x%04x [%u]\n", time, addr, data, changes);
				fflush (stdout);
			}
		}
		if (sleepUs)
			usleep (sleepUs);
	}
	printf("\n");

	pixi_spiClose (&dev);
	if (result < 0)
		PIO_ERROR(-result, "SPI read-write failed");

	return result;
}

static int spiScanFn (const Command* command, uint argc, char* argv[])
{
	if (argc < 2 || argc > 5)
		return commandUsageError (command);

	uint channel = 0;
	uint low     = 0;
	uint high    = 255;
	uint sleepUs = 0;
	channel = pixi_parseLong (argv[1]);
	if (argc > 2)
		low     = pixi_parseLong (argv[2]);
	if (argc > 3)
		high    = pixi_parseLong (argv[3]);
	if (argc > 4)
		sleepUs = pixi_parseLong (argv[4]);

	return scanSpi (channel, low, high, sleepUs);
}
static Command spiScanCmd =
{
	.name        = "spi-scan",
	.description = "continually scan an SPI channel (all addresses)",
	.usage       = "usage: %s CHANNEL [LOW] [HIGH] [SLEEP-US]",
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
	&spiTransferCmd,
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
