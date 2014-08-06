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
#include <libpixi/util/file.h>
#include <libpixi/util/string.h>
#include "Command.h"
#include "log.h"
#include <stdio.h>
#include <fcntl.h>

enum
{
	FlashCapacity   =     524288, // 4 megabit
	FlashPageSize   =        256, // bytes
	FlashId         =   0x202013
};

enum Instructions
{
	// name                    value // address-bytes, dummy-bytes, data-bytes
	WriteEnable              = 0x06, // 0,  0,  0
	WriteDisable             = 0x04, // 0,  0,  0
	ReadIdentification       = 0x9F, // 0,  0,1-3 returns 202013 (manufacturer=20h,memory type=20h,capacity=13h)
	ReadStatusRegister       = 0x05, // 0,  0,1-∞ see enum StatusBits
	WriteStatusRegister      = 0x01, // 0,  0,  1
	ReadDataBytes            = 0x03, // 3,  0,1-∞
	ReadDataBytesFast        = 0x0B, // 3,  1,1-∞
	PageProgram              = 0x02, // 3,  0,1-256
	SectorErase              = 0xD8, // 3,  0,  0
	BulkErase                = 0xC7, // 0,  0,  0
	DeepPowerDown            = 0xB9, // 0,  0,  0
	ReleaseFromDeepPowerDown = 0xAB  // 0,  0,  0, or 0,3,1-∞ for 'and read electronic signature'
};

enum StatusBits
{
	WriteInProgress            = 1 << 0,
	WriteEnableLatch           = 1 << 1,
	BlockProtect0              = 1 << 2,
	BlockProtect1              = 1 << 3,
	BlockProtect2              = 1 << 4,
	// 0 = 1 << 5,
	// 0 = 1 << 6,
	StatusRegisterWriteProtect = 1 << 7
};

static int flashOpen (SpiDevice* device)
{
	int result = pixi_spiOpen (0, PixiSpiSpeed, device);
	if (result < 0)
		PIO_ERROR(-result, "Couldn't open flash SPI channel");
	return result;
}

static int flashReadStatus (SpiDevice* device)
{
	uint8 tx[2] = {
		ReadStatusRegister,
		0
	};
	uint8 rx[2];
	int result = pixi_spiReadWrite (device, tx, rx, sizeof (tx));
	if (result < 0)
	{
		PIO_ERROR(-result, "Flash SPI read of status register failed");
		return result;
	}
	return rx[1];
}

static int flashReadId (SpiDevice* device)
{
	uint8 tx[4] = {
		ReadIdentification,
		0,
		0,
		0
	};
	uint8 rx[4];
	int result = pixi_spiReadWrite (device, tx, rx, sizeof (tx));
	if (result < 0)
	{
		PIO_ERROR(-result, "Flash SPI read of identification failed");
		return result;
	}
	uint id = (rx[1] << 16) | (rx[2] << 8) | rx[3];
	return id;
}

static int checkFlashId (SpiDevice* device)
{
	int result = flashReadId (device);
	if (result < 0)
	{
		PIO_ERROR(-result, "Couldn't read flash ID");
		return result;
	}
	if (result != FlashId)
	{
		PIO_LOG_WARN("Could not identify flash device (id=0x%06x, expected 0x%06x)", result, FlashId);
		PIO_LOG_WARN("Is flash communication configured? Proceeding anyway...");
//		return -ENODEV; // TODO: more suitable error code?
	}
	return 0;
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
		ReleaseFromDeepPowerDown,
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

	int id = flashReadId (&dev);
	pixi_spiClose (&dev);
	if (id < 0)
		return id;

	// Note: id contains useful info, like flash capacity
	printf ("0x%06X\n", id);
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

	checkFlashId (&dev);

	int status = flashReadStatus (&dev);
	pixi_spiClose (&dev);
	if (status < 0)
		return status;

	printf ("0x%02X\n", status);
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
	if (argc != 4)
		return commandUsageError (command);

	uint address = pixi_parseLong (argv[1]);
	uint length  = pixi_parseLong (argv[2]);
	const char* filename = argv[3];
	if (address >= FlashCapacity)
	{
		PIO_LOG_ERROR("Address 0x%x exceeds flash capacity of 0x%x", address, FlashCapacity);
		return -EINVAL;
	}
	uint capacityFromAddress = FlashCapacity - address;
	if (length > capacityFromAddress)
	{
		PIO_LOG_WARN("Length of 0x%x exceeds available capacity of 0x%x from address 0x%x", length, capacityFromAddress, address);
		length = capacityFromAddress;
	}

	SpiDevice dev = SPI_DEVICE_INIT;
	int result = flashOpen (&dev);
	if (result < 0)
		return result;

	checkFlashId (&dev);

	int output = pixi_open (filename, O_CREAT | O_TRUNC | O_WRONLY, 0666);
	if (output < 0)
	{
		PIO_ERROR(-output, "Failed to open output filename [%s]", filename);
		pixi_spiClose (&dev);
		return output;
	}

	// Max SPI transfer size is 4096. Since a few bytes are needed for the request,
	// use a max block size of 2048.
	// Alternatively, could do a transfer sequence keeping CS low between transfers,
	// but this way we get to monitor what's happening
	const uint blockSize = 2048;
	const uint headerSize = 4; // command + 3 address bytes
	const uint bufferSize = blockSize + headerSize;
	uint8 tx[bufferSize];
	uint8 rx[bufferSize];
	memset (tx, 0, sizeof (tx));

	uint total = 0;
	while (length > 0)
	{
		uint size = length > blockSize ? blockSize : length;
		tx[0] = ReadDataBytes;
		tx[1] = address >> 16;
		tx[2] = address >>  8;
		tx[3] = address >>  0;

		result = pixi_spiReadWrite (&dev, tx, rx, headerSize + size);
		if (result < 0)
		{
			PIO_ERROR(-result, "SPI read/write failed");
			break;
		}
		result = pixi_write (output, rx + headerSize, size);
		if (result < 0)
		{
			PIO_ERROR(-result, "Failed to write to output file [%s]", filename);
			break;
		}
		else if (result != (int) size)
		{
			PIO_LOG_ERROR("Short write to output file");
			result = -EIO;
			break;
		}
		printf ("\raddress=0x%x size=%u total=%u ", address, size, total);
		total   += size;
		address += size;
		length  -= size;
		fflush (stdout);
	}
	printf ("\n");
	pixi_close (output);
	pixi_spiClose (&dev);

	return result;
}
static Command flashReadMemoryCmd =
{
	.name        = "flash-read",
	.description = "read flash memory",
	.usage       = "usage: %s ADDRESS LENGTH OUTPUT-FILE",
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
