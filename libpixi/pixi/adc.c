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

#include <libpixi/pixi/adc.h>
#include <libpixi/pixi/spi.h>
#include <libpixi/util/log.h>
#include <unistd.h>

static SpiDevice adcSpi = SPI_DEVICE_INIT;

int pixi_adcOpen (void)
{
	// TODO: instead rejecting if previously open,
	// do ref-counting of open count?
	LIBPIXI_PRECONDITION(adcSpi.fd < 0);
	int result = pixi_spiOpen (PixiAdcSpiChannel, PixiAdcSpiSpeed, &adcSpi);
	if (result < 0)
		LIBPIXI_ERROR(-result, "Cannot open SPI channel to PiXi ADC");
	return result;
}

int pixi_adcClose (void)
{
	LIBPIXI_PRECONDITION(adcSpi.fd >= 0);
	return pixi_spiClose (&adcSpi);
}

int pixi_adcRead (uint adcChannel)
{
	LIBPIXI_PRECONDITION(adcChannel < PixiAdcMaxChannels);

	uint8_t buffer[3] = {
		6, // TODO: what is this?
		adcChannel << 6,
		0
	};
	LIBPIXI_LOG_TRACE("pixi_pixiAdcRead channel=%u, writing %x %x %x", adcChannel, buffer[0], buffer[1], buffer[2]);
	int result = pixi_spiReadWrite (&adcSpi, buffer, buffer, sizeof (buffer));
	LIBPIXI_LOG_TRACE("pixi_pixiAdcRead result=%d, read %x %x %x", result, buffer[0], buffer[1], buffer[2]);

	// Apparently this is useful for bringing CS down. Whatever that means
	read (adcSpi.fd, buffer, 0);

	if (result < 0)
		return result;
	return (buffer[1] << 8) | buffer[2];
}
