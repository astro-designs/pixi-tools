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

#include <libpixi/pi/spi.h>
#include <libpixi/util/file.h>
#include <libpixi/util/log.h>
#include <errno.h>
#include <fcntl.h>
#include <linux/spi/spidev.h>
#include <string.h>
#include <sys/ioctl.h>
#include <unistd.h>

static const char* spiDeviceNames[] = {
	"/dev/spidev0.0",
	"/dev/spidev0.1"
};

int pixi_spiOpen (uint channel, uint speed, SpiDevice* device)
{
	LIBPIXI_PRECONDITION(channel < ARRAY_COUNT(spiDeviceNames));
	LIBPIXI_PRECONDITION_NOT_NULL(device);

	*device = SpiDeviceInit;
	const char* name = spiDeviceNames[channel];
	int fd = pixi_open (name, O_RDWR, 0);
	if (fd < 0)
		return fd;

	LIBPIXI_LOG_DEBUG("Opened SPI name=%s fd=%d, speed=%u", name, fd, speed);
	int mode = 0;
	int result = ioctl (fd, SPI_IOC_WR_MODE, &mode);
	if (result < 0)
	{
		int err = errno;
		LIBPIXI_ERRNO_ERROR("failed to set SPI mode");
		pixi_close (fd);
		return -err;
	}

	uint bits = 8;
	result = ioctl (fd, SPI_IOC_WR_BITS_PER_WORD, &bits);
	if (result < 0)
	{
		int err = errno;
		LIBPIXI_ERRNO_ERROR("failed to set SPI bits-per-word");
		pixi_close (fd);
		return -err;
	}

	uint _speed = speed;
	result = ioctl (fd, SPI_IOC_WR_MAX_SPEED_HZ, &_speed);
	if (result < 0)
	{
		int err = errno;
		LIBPIXI_ERRNO_ERROR("failed to set SPI max speed to %u", speed);
		pixi_close (fd);
		return -err;
	}
	device->fd = fd;
	device->speed = speed;
	device->delay = 0;
	device->bitsPerWord = 8;

	return 0;
}

int pixi_spiClose (SpiDevice* device)
{
	LIBPIXI_PRECONDITION_NOT_NULL(device);

	LIBPIXI_LOG_DEBUG("Closing SPI device fd=%d", device->fd);
	int result = pixi_close (device->fd);
	*device = SpiDeviceInit;
	return result;
}

int pixi_spiReadWrite (SpiDevice* device, const void* txBuffer, void* rxBuffer, size_t bufferSize)
{
	LIBPIXI_PRECONDITION_NOT_NULL(device);
	LIBPIXI_PRECONDITION(device->fd >= 0);
	LIBPIXI_PRECONDITION_NOT_NULL(txBuffer);
	LIBPIXI_PRECONDITION_NOT_NULL(rxBuffer);

	struct spi_ioc_transfer transfer = {
		.tx_buf        = (intptr_t) txBuffer,
		.rx_buf        = (intptr_t) rxBuffer,
		.len           = bufferSize,
		.speed_hz      = device->speed,
		.delay_usecs   = device->delay,
		.bits_per_word = device->bitsPerWord,
		.cs_change     = 0
	};

	LIBPIXI_LOG_TRACE("pixi_spiReadWrite of fd=%d, bufferSize=%zu", device->fd, bufferSize);
	int result = ioctl (device->fd, SPI_IOC_MESSAGE(1), &transfer);
	if (result < 0)
	{
		int err = errno;
		LIBPIXI_ERRNO_ERROR("pixi_spiReadWrite failed");
		return -err;
	}
	return 0;

}
