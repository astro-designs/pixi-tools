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

#ifndef libpixi_pi_spi_h__included
#define libpixi_pi_spi_h__included


#include <libpixi/common.h>
#include <stddef.h>

LIBPIXI_BEGIN_DECLS

///@defgroup PiSpi Raspberry Pi SPI interface
///@{

typedef struct SpiDevice
{
	int fd;          ///< file descriptor
	int speed;       ///< /Hz
	int delay;       ///< /microsecs
	int bitsPerWord;
} SpiDevice;

#define SPI_DEVICE_INIT {-1, 0, 0, 0}
static const SpiDevice SpiDeviceInit = SPI_DEVICE_INIT;

///	Open the SPI device @c channel, also setting @c speed.
///	On success, the fields of @c device are filled out, and
///	you will need to subsequently call @ref pixi_spiClose().
///	@return 0 on success, or -errno on error
int pixi_spiOpen (uint channel, uint speed, SpiDevice* device);

///	Close an SPI device opened via pixi_spiOpen()
///	@return 0 on success, or -errno on error
int pixi_spiClose (SpiDevice* device);

///	Perform a read/write on an SPI device opened via pixi_spiOpen()
///	@return 0 on success, or -errno on error
int pixi_spiReadWrite (SpiDevice* device, const void* outputBuffer, void* inputBuffer, size_t bufferSize);

///@} defgroup

LIBPIXI_END_DECLS

#endif // !defined libpixi_pi_spi_h__included
