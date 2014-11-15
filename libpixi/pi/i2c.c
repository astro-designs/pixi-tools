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
#include <assert.h>
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


int pixi_i2cWriteRead (I2cDevice* device, const void* txBuffer, size_t txSize, void* rxBuffer, size_t rxSize)
{
	LIBPIXI_PRECONDITION_NOT_NULL(device);
	LIBPIXI_PRECONDITION(device->fd >= 0);
	LIBPIXI_PRECONDITION(device->address < 1024);

	I2cMessage messages[2];
	uint count = 0;
	if (txBuffer)
	{
		messages[count].address = device->address;
		messages[count].flags   = I2C_M_TEN;
		messages[count].length  = txSize;
		messages[count].buffer  = (void*) txBuffer;
		count++;
	}
	if (rxBuffer)
	{
		messages[count].address = device->address;
		messages[count].flags   = I2C_M_TEN | I2C_M_RD;
		messages[count].length  = rxSize;
		messages[count].buffer  = rxBuffer;
		count++;
	}

	return pixi_i2cMultiOp (device, messages, count);
}

int pixi_i2cMultiOp (I2cDevice* device, I2cMessage* messages, size_t count)
{
	LIBPIXI_STATIC_ASSERT (sizeof (struct i2c_msg) == sizeof (I2cMessage), "struct i2c_msg must match I2cMessage");
	LIBPIXI_STATIC_ASSERT (offsetof(struct i2c_msg, addr ) == offsetof(I2cMessage, address), "struct i2c_msg must match I2cMessage");
	LIBPIXI_STATIC_ASSERT (offsetof(struct i2c_msg, flags) == offsetof(I2cMessage, flags  ), "struct i2c_msg must match I2cMessage");
	LIBPIXI_STATIC_ASSERT (offsetof(struct i2c_msg, len  ) == offsetof(I2cMessage, length ), "struct i2c_msg must match I2cMessage");
	LIBPIXI_STATIC_ASSERT (offsetof(struct i2c_msg, buf  ) == offsetof(I2cMessage, buffer ), "struct i2c_msg must match I2cMessage");
	LIBPIXI_STATIC_ASSERT (I2C_M_RD == I2cMsgRead, "struct i2c_msg must match I2cMessage");

	LIBPIXI_PRECONDITION_NOT_NULL(device);
	LIBPIXI_PRECONDITION(device->fd >= 0);
	LIBPIXI_PRECONDITION(device->address < 1024);
	LIBPIXI_PRECONDITION_NOT_NULL(messages);

	for (uint i = 0; i < count; i++)
		messages[i].address = device->address;

	struct i2c_rdwr_ioctl_data msgset = {
		.msgs  = (struct i2c_msg*) messages,
		.nmsgs = count
	};

	int result = ioctl (device->fd, I2C_RDWR, &msgset);
	if (result < 0)
	{
		int err = errno;
		LIBPIXI_ERROR(err, "ioctl I2C_RDWR failed");
		return -err;
	}
	if (pixi_isLogLevelEnabled (LogLevelTrace))
	{
		char hex[4096];
		LIBPIXI_LOG_TRACE("Completed %zu message i2c transfer for address 0x%02x", count, device->address);
		for (uint i = 0; i < count; i++)
		{
			pixi_hexEncode (messages[i].buffer, messages[i].length, hex, sizeof(hex), ' ', "");
			const char* type = (messages[i].flags & I2cMsgRead) ? "read" : "write";
			LIBPIXI_LOG_TRACE("msg %u %-5s [%s]", i, type, hex);
		}
	}
	return 0;
}

