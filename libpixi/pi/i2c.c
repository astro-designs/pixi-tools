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

#include <libpixi/pi/i2c.h>
#include <libpixi/util/file.h>
#include <libpixi/util/log.h>
#include <libpixi/util/string.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/ioctl.h>
#include <unistd.h>
#include <linux/i2c-dev.h>

const char spiDevice[] = "/dev/i2c-%d";

int pixi_i2cOpen (uint channel, uint address)
{
	I2cDevice device = I2C_DEVICE_INIT;
	int result = pixi_i2cOpen2 (channel, address, &device);
	if (result < 0)
		return result;
	return device.fd;
}

int pixi_i2cOpen2 (uint channel, uint address, I2cDevice* device)
{
	LIBPIXI_PRECONDITION(channel < 16);
	LIBPIXI_PRECONDITION(address < 1024);

	char filename[40];
	snprintf (filename, sizeof (filename), spiDevice, channel);
	int fd = pixi_open (filename, O_RDWR, 0);
	if (fd < 0)
	{
		LIBPIXI_ERROR_DEBUG(-fd, "failed to open i2c device [%s] failed", filename);
		return fd;
	}

	LIBPIXI_LOG_DEBUG("Opened i2c name=%s fd=%d", filename, fd);
	int result = ioctl (fd, I2C_SLAVE, address);
	if (result < 0)
	{
		int err = errno;
		LIBPIXI_ERROR(err, "failed to set i2c address to %u", address);
		pixi_close (fd);
		return -err;
	}
	memset (device, 0, sizeof (*device));
	device->fd      = fd;
	device->address = address;

	return fd;
}


int pixi_i2cClose (I2cDevice* device)
{
	LIBPIXI_PRECONDITION_NOT_NULL(device);

	LIBPIXI_LOG_DEBUG("Closing i2c device fd=%d", device->fd);
	int result = pixi_close (device->fd);
	*device = I2cDeviceInit;
	return result;
}


int pixi_i2cReadWrite (I2cDevice* device, const void* txBuffer, size_t txSize, void* rxBuffer, size_t rxSize)
{
	LIBPIXI_PRECONDITION_NOT_NULL(device);
	LIBPIXI_PRECONDITION(device->fd >= 0);
	LIBPIXI_PRECONDITION(device->address < 1024);
	LIBPIXI_PRECONDITION_NOT_NULL(txBuffer);
	LIBPIXI_PRECONDITION_NOT_NULL(rxBuffer);

	struct i2c_msg msgs[2] = {
		{.addr = device->address, .len = txSize, .buf = (void*) txBuffer, .flags = I2C_M_TEN},
		{.addr = device->address, .len = rxSize, .buf =         rxBuffer, .flags = I2C_M_TEN | I2C_M_RD},
	};

	struct i2c_rdwr_ioctl_data msgset = {
		.msgs  = msgs,
		.nmsgs = 2
	};

	int result = ioctl (device->fd, I2C_RDWR, &msgset);
	if (result < 0)
	{
		int err = errno;
		LIBPIXI_ERROR(err, "ioctl I2C_RDWR failed");
		return -err;
	}
	return 0;
}
