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

#include <libpixi/pixi/flash.h>
#include <libpixi/util/file.h>
#include <libpixi/util/string.h>
#include "Command.h"
#include "log.h"
#include <stdio.h>
#include <fcntl.h>

static int checkFlashId (void)
{
	int result = pixi_flashReadId();
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

	int result = pixi_flashOpen();
	if (result < 0)
		return result;

	result = pixi_flashRdpReadSig();
	pixi_flashClose();
	if (result < 0)
	{
		PIO_ERROR(-result, "Release flash from deep power down and read electronic signature failed");
		return result;
	}
	printf ("Result=0x%02x\n", result);
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

	int result = pixi_flashOpen();
	if (result < 0)
		return result;

	int id = pixi_flashReadId();
	pixi_flashClose();
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

	int result = pixi_flashOpen();
	if (result < 0)
		return result;

	checkFlashId();

	int status = pixi_flashReadStatus();
	pixi_flashClose();
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

	int result = pixi_flashOpen();
	if (result < 0)
		return result;

	checkFlashId();

	int output = pixi_open (filename, O_CREAT | O_TRUNC | O_WRONLY, 0666);
	if (output < 0)
	{
		PIO_ERROR(-output, "Failed to open output filename [%s]", filename);
		pixi_flashClose();
		return output;
	}

	uint8 buffer[FlashCapacity];
	printf ("Reading from flash address=0x%x, length=0x%x\n", address, length);
	result = pixi_flashReadMemory (address, buffer, length);
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
	pixi_flashClose();

	return result;
}
static Command flashReadMemoryCmd =
{
	.name        = "flash-read",
	.description = "read flash memory",
	.usage       = "usage: %s ADDRESS LENGTH OUTPUT-FILE",
	.function    = flashReadMemoryFn
};

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

	result = pixi_flashOpen();
	if (result < 0)
		return result;

	checkFlashId();

	if (erase)
	{
		printf ("Erasing sectors in region address=0x%x, length=0x%x\n", address, length);
		result = pixi_flashEraseSectors (address, length);
		if (result < 0)
			PIO_LOG_ERROR("Sector erase failed");
	}
	if (result >= 0)
	{
		printf ("Writing to flash address=0x%x, length=0x%x\n", address, length);
		result = pixi_flashWriteMemory (address, buffer, length);
		if (result < 0)
			PIO_LOG_ERROR("Flash write failed");
	}
	if (result > 0)
	{
		printf ("Finished writing to flash\n");
		printf ("Verifying: reading from flash\n");
		int written = result;
		char check[written];
		result = pixi_flashReadMemory (address, check, written);
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

	pixi_flashClose();

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

	int result = pixi_flashOpen();
	if (result < 0)
		return result;

	checkFlashId();

	printf ("Erasing flash sectors\n");
	result = pixi_flashEraseSectors (address, length);
	if (result < 0)
		PIO_ERROR(-result, "erasing sectors failed");
	else
		printf ("Erasing 0x%02x bytes of flash\n", result);

	pixi_flashClose();

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

	int result = pixi_flashOpen();
	if (result < 0)
		return result;

	checkFlashId();

	printf ("Erasing all flash memory\n");
	result = pixi_flashBulkErase();
	pixi_flashClose();
	if (result < 0)
		PIO_ERROR(-result, "Bulk erase failed");
	else
		printf ("Finished erasing flash memory\n");


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
