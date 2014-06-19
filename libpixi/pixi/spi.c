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

#include <libpixi/pixi/spi.h>
#include <libpixi/util/log.h>
#include <linux/spi/spidev.h>
#include <sys/ioctl.h>

static SpiDevice pixiSpi = SPI_DEVICE_INIT;

int pixi_openPixi (void)
{
	// TODO: instead rejecting if previously open,
	// do ref-counting of open count?
	LIBPIXI_PRECONDITION(pixiSpi.fd < 0);
	int result = pixi_spiOpen (PixiSpiChannel, PixiSpiSpeed, &pixiSpi);
	if (result < 0)
		LIBPIXI_ERROR(-result, "Cannot open SPI channel to pixi");
	return result;
}

int pixi_closePixi (void)
{
	LIBPIXI_PRECONDITION(pixiSpi.fd >= 0);
	return pixi_spiClose (&pixiSpi);
}

static int readWriteValue16 (uint function, uint address, uint16 value)
{
	uint8_t buffer[4] = {
		address,
		function,
		(value & 0xFF00) >> 8,
		(value & 0x00FF)
	};
	int result = pixi_spiReadWrite (&pixiSpi, buffer, buffer, sizeof (buffer));
	if (result < 0)
		return result;
	return (buffer[2] << 8) | buffer[3];
}

int pixi_registerRead (uint address)
{
	int result = readWriteValue16 (PixiSpiEnableRead16, address, 0);
	LIBPIXI_LOG_DEBUG("pixi_registerRead address=0x%02x result=%d", address, result);
	return result;
}

int pixi_registerWrite (uint address, ushort value)
{
	int result = readWriteValue16 (PixiSpiEnableWrite16, address, value);
	LIBPIXI_LOG_DEBUG("pixi_registerWrite address=0x%02x value=0x%04x result=%d", address, value, result);
	return result;
}

int pixi_registerWriteMasked (uint address, ushort value, ushort mask)
{
	int previous = pixi_registerRead (address);
	if (previous < 0)
		return previous;
	ushort masked = (value & mask) | (previous & ~mask);
	int result = pixi_registerWrite (address, masked);
	if (result < 0)
		return result;
	return previous;
}

int pixi_multiRegisterOp (RegisterOp* operations, uint opCount)
{
	LIBPIXI_PRECONDITION(pixiSpi.fd >= 0);
	LIBPIXI_PRECONDITION_NOT_NULL(operations);
	LIBPIXI_PRECONDITION(opCount < 256);

	struct spi_ioc_transfer transfers[opCount];
	for (uint i = 0; i < opCount; i++)
	{
		operations[i]._valueHi = operations[i].value >> 8;
		operations[i]._valueLo = operations[i].value;
		transfers[i].tx_buf        = (intptr_t) &operations[i].address;
		transfers[i].rx_buf        = transfers[i].tx_buf;
		transfers[i].len           = 4;
		transfers[i].speed_hz      = pixiSpi.speed;
		transfers[i].delay_usecs   = pixiSpi.delay;
		transfers[i].bits_per_word = pixiSpi.bitsPerWord;
		transfers[i].cs_change     = 1;
	}
	LIBPIXI_LOG_TRACE("pixi_multiRegisterOp of fd=%d, count=%u", pixiSpi.fd, opCount);
	int result = ioctl (pixiSpi.fd, SPI_IOC_MESSAGE(opCount), &transfers);
	if (result < 0)
	{
		int err = errno;
		LIBPIXI_ERRNO_ERROR("pixi_multiRegisterOp failed");
		return -err;
	}
	for (uint i = 0; i < opCount; i++)
		operations[i].value = (((uint) operations[i]._valueHi) << 8 ) | operations[i]._valueLo;

	return 0;
}
