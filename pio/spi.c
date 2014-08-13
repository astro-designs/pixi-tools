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

#include <libpixi/pixi/registers.h>
#include <libpixi/pixi/spi.h>
#include <libpixi/pixi/simple.h>
#include <libpixi/util/string.h>
#include "common.h"
#include "log.h"
#include <stdio.h>
#include <stdlib.h>
#include <sys/time.h>
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

static int spiSetGet (bool writeMode, uint address, uint data)
{
	pixiOpenOrDie();

	RegisterOp op = {
		.address  = address,
		.function = (writeMode ? PixiSpiEnableWrite16 : PixiSpiEnableRead16),
		.value    = data
	};
	int result = multiRegisterOp (&op, 1);
	if (result < 0)
		PIO_ERROR(-result, "SPI read-write failed");

	PIO_LOG_INFO("SPI returned: 0x%02x 0x%02x 0x%04x", op.address, op.function, op.value);
	if (!writeMode)
		printf ("%u\n", op.value);
	pixiClose();
	return result;
}

static int spiSetFn (const Command* command, uint argc, char* argv[])
{
	if (argc != 3)
		return commandUsageError (command);

	uint address = pixi_parseLong (argv[1]);
	uint data    = pixi_parseLong (argv[2]);

	return spiSetGet (true, address, data);
}
static Command spiSetCmd =
{
	.name        = "spi-set",
	.description = "write a value to the pixi over spi",
	.usage       = "usage: %s ADDRESS VALUE",
	.function    = spiSetFn
};

static int spiGetFn (const Command* command, uint argc, char* argv[])
{
	if (argc < 2 || argc > 3)
		return commandUsageError (command);

	uint address = pixi_parseLong (argv[1]);
	uint data = 0;
	if (argc > 2)
		data    = pixi_parseLong (argv[2]);

	return spiSetGet (false, address, data);
}
static Command spiGetCmd =
{
	.name        = "spi-get",
	.description = "read a value from the pixi over spi",
	.usage       = "usage: %s ADDRESS [NULL-DATA]",
	.function    = spiGetFn
};


static int monitorSpi (uint address)
{
	pixiOpenOrDie();

	int result = 0;
	uint previous = -1;
	uint changes = 0;
	while (true)
	{
		result = registerRead (address);;
		if (result < 0)
			break;

		uint data = result;
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

	pixiClose();

	if (result < 0)
		PIO_ERROR(-result, "SPI read-write failed");

	return result;
}

static int spiMonitorFn (const Command* command, uint argc, char* argv[])
{
	if (argc != 2)
		return commandUsageError (command);

	uint address = pixi_parseLong (argv[1]);

	return monitorSpi (address);
}
static Command spiMonitorCmd =
{
	.name        = "spi-monitor",
	.description = "monitor a PiXi register via SPI",
	.usage       = "usage: %s ADDRESS",
	.function    = spiMonitorFn
};


static int scanSpi (uint low, uint high, uint sleepUs)
{
	pixiOpenOrDie();

	const uint count = high - low + 1;
	uint memory[count];
	memset (memory, 0, count * sizeof (uint));
	uint iterations = 0;
	uint changes    = 0;
	int result = 0;

	while (true)
	{
		struct timeval tod;
		gettimeofday (&tod, NULL);

		RegisterOp ops[count];
		for (uint i = 0; i < count; i++)
		{
			ops[i].address  = low + i;
			ops[i].function = PixiSpiEnableRead16;
			ops[i].value    = 0;
		}
		result = pixi_multiRegisterOp (ops, count);
		if (result < 0)
		{
			LIBPIXI_ERROR(-result, "pixi_multiRegisterOp failed");
			break;
		}
		bool haveTime = false;
		char time[26];
		for (uint i = 0; i < count; i++)
		{
			if (memory[i] != ops[i].value)
			{
				if (!haveTime)
				{
					pixi_formatTimeval (&tod, time, sizeof (time));
					haveTime = true;
				}
				changes++;
				memory[i] = ops[i].value;
				printf("%s: 0x%02x 0x%04x [iterations=%u changes=%u]\n",
					time, ops[i].address, ops[i].value, iterations, changes);
				fflush (stdout);
			}
		}
		if (sleepUs)
			usleep (sleepUs);
		iterations++;
	}
	printf("\n");

	pixiClose();
	if (result < 0)
		PIO_ERROR(-result, "SPI read-write failed");

	return result;
}

static int spiScanFn (const Command* command, uint argc, char* argv[])
{
	if (argc < 1 || argc > 4)
		return commandUsageError (command);

	uint low     = 0;
	uint high    = 255;
	uint sleepUs = 0;
	if (argc > 1)
		low     = pixi_parseLong (argv[1]);
	if (argc > 2)
		high    = pixi_parseLong (argv[2]);
	if (argc > 3)
		sleepUs = pixi_parseLong (argv[3]);

	if (high < low || low > 255 || high > 255)
	{
		PIO_LOG_ERROR("Registers out of range");
		return -EINVAL;
	}
	return scanSpi (low, high, sleepUs);
}
static Command spiScanCmd =
{
	.name        = "spi-scan",
	.description = "continually scan a range of PiXi registers via SPI",
	.usage       = "usage: %s [LOW] [HIGH] [SLEEP-US]",
	.function    = spiScanFn
};

static int monitorButtonsFn (const Command* command, uint argc, char* argv[])
{
	LIBPIXI_UNUSED(argv);
	if (argc != 1)
		return commandUsageError (command);

	return monitorSpi (Pixi_Switch_in);
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

static void PIO_CONSTRUCTOR (1020) initGroup (void)
{
	addCommandGroup (&pixiSpiGroup);
}
