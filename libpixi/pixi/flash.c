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
#include <libpixi/pixi/spi.h>
#include <libpixi/util/log.h>
#include <libpixi/util/string.h>
#include <string.h>
#include <stdio.h>
#include <fcntl.h>

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

int flashOpen (SpiDevice* device)
{
	int result = pixi_spiOpen (0, PixiSpiSpeed, device);
	if (result < 0)
		LIBPIXI_ERROR(-result, "Couldn't open flash SPI channel");
	return result;
}

int pixi_flashRdpReadSig (SpiDevice* device)
{
	uint8 tx[5] = {
		ReleaseFromDeepPowerDown,
		0,
		0,
		0,
		0
	};
	uint8 rx[5];
	int result = pixi_spiReadWrite (device, tx, rx, sizeof (tx));
	if (result < 0)
	{
		LIBPIXI_ERROR(-result, "SPI read/write failed");
		return result;
	}
	if (pixi_isLogLevelEnabled (LogLevelDebug))
	{
		char hex[20];
		pixi_hexEncode (rx, sizeof (rx), hex, sizeof (hex), ' ', "");
		LIBPIXI_LOG_DEBUG("RdpReadSig rx=[%s]", hex);
	}
	return rx[4];

}

int flashReadStatus (SpiDevice* device)
{
	uint8 tx[2] = {
		ReadStatusRegister,
		0
	};
	uint8 rx[2];
	int result = pixi_spiReadWrite (device, tx, rx, sizeof (tx));
	if (result < 0)
	{
		LIBPIXI_ERROR(-result, "Flash SPI read of status register failed");
		return result;
	}
	return rx[1];
}

int flashReadId (SpiDevice* device)
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
		LIBPIXI_ERROR(-result, "Flash SPI read of identification failed");
		return result;
	}
	uint id = (rx[1] << 16) | (rx[2] << 8) | rx[3];
	return id;
}

int flashReadMemory (SpiDevice* device, uint address, void* buffer, uint length)
{
	LIBPIXI_PRECONDITION_NOT_NULL(buffer);

	if (address >= FlashCapacity)
	{
		LIBPIXI_LOG_ERROR("Address 0x%x exceeds flash capacity of 0x%x", address, FlashCapacity);
		return -EINVAL;
	}
	uint capacityFromAddress = FlashCapacity - address;
	if (length > capacityFromAddress)
	{
		LIBPIXI_LOG_WARN("Length of 0x%x exceeds available capacity of 0x%x from address 0x%x", length, capacityFromAddress, address);
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
			LIBPIXI_ERROR(-result, "SPI read/write failed");
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

static int flashSendWrite (SpiDevice* device, const void* tx, void* rx, uint size)
{
	uint8 writeEnable = WriteEnable;
	int result = pixi_spiReadWrite (device, &writeEnable, &writeEnable, sizeof (writeEnable));
	if (result < 0)
	{
		LIBPIXI_ERROR(-result, "Flash SPI write-enable failed");
		return result;
	}
	result = flashReadStatus (device);
	if (!(result & WriteEnableLatch))
	{
		LIBPIXI_LOG_ERROR("WriteEnable bit not in status register");
		return -EIO;
	}

	if (pixi_isLogLevelEnabled (LogLevelTrace))
	{
		char hex[1 + (size*3)];
		pixi_hexEncode (tx, size, hex, sizeof (hex), ' ', "");
		LIBPIXI_LOG_TRACE("Sending 'write' [%s]", hex);
	}

	result = pixi_spiReadWrite (device, tx, rx, size);
	if (result < 0)
	{
		LIBPIXI_ERROR(-result, "Flash SPI page program failed");
		return result;
	}
	for (uint i = 0; i < 100000; i++)
	{
		result = flashReadStatus (device);
		if (result < 0)
			return result;
		if (!(result & WriteInProgress))
		{
			LIBPIXI_LOG_TRACE("Waited %u iterations for write to complete", i);
			return 0;
		}
	}
	if (result & WriteInProgress)
	{
		LIBPIXI_LOG_ERROR("Waited too long for write to finish");
		return -EIO;
	}
	return 0;
}

int flashEraseSectors (SpiDevice* device, uint address, uint length)
{
	if (address >= FlashCapacity)
	{
		LIBPIXI_LOG_ERROR("Address 0x%x exceeds available flash capacity of 0x%x", address, FlashCapacity);
		return -EINVAL;
	}
	if (length == 0)
		return 0;

	uint capacityFromAddress = FlashCapacity - address;
	if (length > capacityFromAddress)
	{
		LIBPIXI_LOG_WARN("Write request length exceed capacity of 0x%x from address 0x%x", capacityFromAddress, address);
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
		LIBPIXI_LOG_INFO("Erasing sector at address 0x%06x", address);
		result = flashSendWrite (device, tx, tx, sizeof (tx));
		if (result < 0)
			break;

		address += FlashSectorSize;
		result  += FlashSectorSize;
	}

	return result;
}

int pixi_flashBulkErase (SpiDevice* device)
{
	uint8 bulkErase = BulkErase;
	return flashSendWrite (device, &bulkErase, &bulkErase, sizeof (bulkErase));
}

int flashWriteMemory (SpiDevice* device, uint address, const void* buffer, uint length)
{
	if (address >= FlashCapacity)
	{
		LIBPIXI_LOG_ERROR("Address 0x%x exceeds available flash capacity of 0x%x", address, FlashCapacity);
		return -EINVAL;
	}
	const char* pbuf = buffer;

	uint capacityFromAddress = FlashCapacity - address;
	if (length > capacityFromAddress)
	{
		LIBPIXI_LOG_WARN("Write request length exceed capacity of 0x%x from address 0x%x", capacityFromAddress, address);
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
		LIBPIXI_LOG_DEBUG("Writing %u bytes at address 0x%06x", size, address);
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
