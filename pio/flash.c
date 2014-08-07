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
	FlashId         =   0x202013,

	FlashSectorSize     = 256 * FlashPageSize, // bytes
	FlashSectorBaseMask = ~(FlashSectorSize - 1) // bytes
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

static int flashReadMemory (SpiDevice* device, uint address, void* buffer, uint length)
{
	PIO_PRECONDITION_NOT_NULL(buffer);

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
	char* output = buffer;

	int result = 0;
	uint total = 0;
	while (length > 0)
	{
		uint size = length > blockSize ? blockSize : length;
		tx[0] = ReadDataBytes;
		tx[1] = address >> 16;
		tx[2] = address >>  8;
		tx[3] = address >>  0;

		result = pixi_spiReadWrite (device, tx, rx, headerSize + size);
		if (result < 0)
		{
			PIO_ERROR(-result, "SPI read/write failed");
			break;
		}
		memcpy (output, rx + headerSize, size);

		output  += size;
		total   += size;
		address += size;
		length  -= size;
		result   = total;
	}

	return result;

}

static int flashReadMemoryFn (const Command* command, uint argc, char* argv[])
{
	if (argc != 4)
		return commandUsageError (command);

	uint address = pixi_parseLong (argv[1]);
	uint length  = pixi_parseLong (argv[2]);
	const char* filename = argv[3];

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

	uint8 buffer[FlashCapacity];
	printf ("Reading from flash address=0x%x, length=0x%x\n", address, length);
	result = flashReadMemory (&dev, address, buffer, length);
	printf ("Finished reading from flash\n");
	if (result >= 0)
	{
		int size = result;
		printf ("Writing to file [%s]\n", filename);
		result = pixi_write (output, buffer, size);
		if (result < 0)
			PIO_ERROR(-result, "Failed to write to output file [%s]", filename);
		else if (result != size)
		{
			PIO_LOG_ERROR("Short write to output file");
			result = -EIO;
		}
		else
			result = 0;
	}
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


static int flashSendWrite (SpiDevice* device, const void* tx, void* rx, uint size)
{
	uint8 writeEnable = WriteEnable;
	int result = pixi_spiReadWrite (device, &writeEnable, &writeEnable, sizeof (writeEnable));
	if (result < 0)
	{
		PIO_ERROR(-result, "Flash SPI write-enable failed");
		return result;
	}
	result = flashReadStatus (device);
	if (!(result & WriteEnableLatch))
	{
		PIO_LOG_ERROR("WriteEnable bit not in status register");
		return -EIO;
	}

	if (pio_isLogLevelEnabled (LogLevelTrace))
	{
		char hex[1 + (size*3)];
		pixi_hexEncode (tx, size, hex, sizeof (hex), ' ', "");
		PIO_LOG_TRACE("Sending 'write' [%s]", hex);
	}

	result = pixi_spiReadWrite (device, tx, rx, size);
	if (result < 0)
	{
		PIO_ERROR(-result, "Flash SPI page program failed");
		return result;
	}
	for (uint i = 0; i < 100000; i++)
	{
		result = flashReadStatus (device);
		if (result < 0)
			return result;
		if (!(result & WriteInProgress))
		{
			PIO_LOG_TRACE("Waited %u iterations for write to complete", i);
			return 0;
		}
	}
	if (result & WriteInProgress)
	{
		PIO_LOG_ERROR("Waited too long for write to finish");
		return -EIO;
	}
	return 0;
}

///	Erase the sectors corresponding to the region defined by @a address, @a length.
static int flashEraseSectors (SpiDevice* device, uint address, uint length)
{
	if (address >= FlashCapacity)
	{
		PIO_LOG_ERROR("Address 0x%x exceeds available flash capacity of 0x%x", address, FlashCapacity);
		return -EINVAL;
	}
	if (length == 0)
		return 0;

	uint capacityFromAddress = FlashCapacity - address;
	if (length > capacityFromAddress)
	{
		PIO_LOG_WARN("Write request length exceed capacity of 0x%x from address 0x%x", capacityFromAddress, address);
		length = capacityFromAddress;
	}

	const uint limit = address + length;
	address &= FlashSectorBaseMask;
	uint8 tx[4];

	int result = 0;
	while (address < limit)
	{
		tx[0] = SectorErase;
		tx[1] = address >> 16;
		tx[2] = address >>  8;
		tx[3] = address >>  0;
		PIO_LOG_INFO("Erasing sector at address 0x%06x", address);
		result = flashSendWrite (device, tx, tx, sizeof (tx));
		if (result < 0)
			break;

		address += FlashSectorSize;
		result  += FlashSectorSize;
	}

	return result;
}

static int flashWriteMemory (SpiDevice* device, uint address, const void* buffer, uint length)
{
	if (address >= FlashCapacity)
	{
		PIO_LOG_ERROR("Address 0x%x exceeds available flash capacity of 0x%x", address, FlashCapacity);
		return -EINVAL;
	}
	const char* pbuf = buffer;

	uint capacityFromAddress = FlashCapacity - address;
	if (length > capacityFromAddress)
	{
		PIO_LOG_WARN("Write request length exceed capacity of 0x%x from address 0x%x", capacityFromAddress, address);
		length = capacityFromAddress;
	}

	const uint blockSize = FlashPageSize;
	const uint headerSize = 4; // command + 3 address bytes
	const uint bufferSize = blockSize + headerSize;
	uint8 tx[bufferSize];
	uint8 rx[bufferSize];

	int result = 0;
	uint total = 0;
	while (length > 0)
	{
		uint pageOffset = address & (FlashPageSize - 1);
		uint pageRemainder = FlashPageSize - pageOffset;
		// Write a page a time. If a write extends beyond page,
		// it wraps around to the beginning of the page.
		uint size = length > pageRemainder ? pageRemainder : length;

		tx[0] = PageProgram;
		tx[1] = address >> 16;
		tx[2] = address >>  8;
		tx[3] = address >>  0;
		memcpy (tx + headerSize, pbuf, size);
		PIO_LOG_DEBUG("Writing %u bytes at address 0x%06x", size, address);
		result = flashSendWrite (device, tx, rx, headerSize + size);
		if (result < 0)
			break;

		total   += size;
		address += size;
		length  -= size;
		pbuf    += size;
		result   = total;
	}

	return result;
}

static int flashEraseWriteMemory (const Command* command, uint argc, char* argv[], bool erase)
{
	if (argc != 3)
		return commandUsageError (command);

	uint address = pixi_parseLong (argv[1]);
	const char* filename = argv[2];
	char buffer[FlashCapacity+1];
	int result = pixi_fileRead (filename, buffer, sizeof (buffer));
	if (result < 0)
	{
		PIO_ERROR(-result, "Failed to open input filename [%s]", filename);
		return result;
	}
	uint length = result;

	SpiDevice dev = SPI_DEVICE_INIT;
	result = flashOpen (&dev);
	if (result < 0)
		return result;

	checkFlashId (&dev);

	if (erase)
	{
		printf ("Erasing sectors in region address=0x%x, length=0x%x\n", address, length);
		result = flashEraseSectors (&dev, address, length);
		if (result < 0)
			PIO_LOG_ERROR("Sector erase failed");
	}
	if (result >= 0)
	{
		printf ("Writing to flash address=0x%x, length=0x%x\n", address, length);
		result = flashWriteMemory (&dev, address, buffer, length);
		if (result < 0)
			PIO_LOG_ERROR("Flash write failed");
	}
	if (result > 0)
	{
		printf ("Finished writing to flash\n");
		printf ("Verifying: reading from flash\n");
		int written = result;
		char check[written];
		result = flashReadMemory (&dev, address, check, written);
		if (result == written)
		{
			printf ("Comparing memory\n");
			result = 0;
			for (int i = 0; i < written; i++)
			{
				if (buffer[i] != check[i])
				{
					PIO_LOG_FATAL ("Wrote to flash, but verification failed at offset 0x%0x", i);
					result = -EIO;
					break;
				}
			}
			if (result == 0)
				printf ("Verified\n");
		}
		else
		{
			PIO_LOG_FATAL ("Wrote to flash, but verify read failed");
			if (result >= 0)
				result = -EIO;
		}
	}

	pixi_spiClose (&dev);

	return result;
}

static int flashWriteMemoryFn (const Command* command, uint argc, char* argv[])
{
	return flashEraseWriteMemory (command, argc, argv, true);
}
static Command flashWriteMemoryCmd =
{
	.name        = "flash-write",
	.description = "write flash memory",
	.usage       = "usage: %s ADDRESS INPUT-FILE",
	.function    = flashWriteMemoryFn
};

static int flashWriteMemoryNoEraseFn (const Command* command, uint argc, char* argv[])
{
	return flashEraseWriteMemory (command, argc, argv, false);
}
static Command flashWriteMemoryNoEraseCmd =
{
	.name        = "flash-write-no-erase",
	.description = "write flash memory without erasing the sectors",
	.usage       = "usage: %s ADDRESS INPUT-FILE",
	.function    = flashWriteMemoryNoEraseFn
};

static int flashEraseSectorsFn (const Command* command, uint argc, char* argv[])
{
	if (argc != 3)
		return commandUsageError (command);

	uint address = pixi_parseLong (argv[1]);
	uint length  = pixi_parseLong (argv[2]);

	SpiDevice dev = SPI_DEVICE_INIT;
	int result = flashOpen (&dev);
	if (result < 0)
		return result;

	checkFlashId (&dev);

	printf ("Erasing flash sectors\n");
	result = flashEraseSectors (&dev, address, length);
	if (result < 0)
		PIO_ERROR(-result, "erasing sectors failed");
	else
		printf ("Erasing 0x%02x bytes of flash\n", result);

	pixi_spiClose (&dev);

	return result;
}
static Command flashEraseSectorsCmd =
{
	.name        = "flash-erase-sectors",
	.description = "erase flash sectors corresponding to a memory region",
	.usage       = "usage: %s ADDRESS LENGTH",
	.function    = flashEraseSectorsFn
};

static int flashEraseFn (const Command* command, uint argc, char* argv[])
{
	LIBPIXI_UNUSED(argv);
	if (argc != 1)
		return commandUsageError (command);

	SpiDevice dev = SPI_DEVICE_INIT;
	int result = flashOpen (&dev);
	if (result < 0)
		return result;

	checkFlashId (&dev);

	printf ("Erasing all flash memory\n");
	uint8 bulkErase = BulkErase;
	result = flashSendWrite (&dev, &bulkErase, &bulkErase, sizeof (bulkErase));
	printf ("Finished erasing flash memory\n");

	pixi_spiClose (&dev);

	return result;
}
static Command flashEraseCmd =
{
	.name        = "flash-erase",
	.description = "erase all flash memory",
	.usage       = "usage: %s",
	.function    = flashEraseFn
};


static const Command* commands[] =
{
	&flashRdpResCmd,
	&flashReadIdCmd,
	&flashReadStatusCmd,
	&flashReadMemoryCmd,
	&flashWriteMemoryCmd,
	&flashWriteMemoryNoEraseCmd,
	&flashEraseSectorsCmd,
	&flashEraseCmd,
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
