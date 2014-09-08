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
#include <libpixi/util/bits.h>
#include <libpixi/util/log.h>
#include <libpixi/util/string.h>
#include <unistd.h>

static int adcReadMCP3204 (uint adcChannel);
static int adcReadADC128S022 (uint adcChannel);

static int (*adcReadImpl) (uint adcChannel) = adcReadADC128S022;
static uint adcChannels = 8;

static SpiDevice adcSpi = SPI_DEVICE_INIT;

int pixi_adcOpen (void)
{
	// TODO: instead rejecting if previously open,
	// do ref-counting of open count?
	LIBPIXI_PRECONDITION(adcSpi.fd < 0);
	int result = pixi_spiOpen (PixiAdcSpiChannel, PixiAdcSpiSpeed, &adcSpi);
	if (result < 0)
		LIBPIXI_ERROR(-result, "Cannot open SPI channel to PiXi ADC");
	// TODO: work out which ADC it is
	return result;
}

int pixi_adcClose (void)
{
	LIBPIXI_PRECONDITION(adcSpi.fd >= 0);
	return pixi_spiClose (&adcSpi);
}

static int adcReadMCP3204 (uint adcChannel)
{
	uint8 tx[3] = {
		6, // start bits (bit 0 is high bit of channel select for MCP3208)
		adcChannel << 6,
		0
	};
	uint8 rx[3] = {0,0,0};
	int result = pixi_spiReadWrite (&adcSpi, tx, rx, sizeof (tx));
	if (result < 0)
		return result;

	uint value = makeUint12 (rx[1], rx[2]);
	if (pixi_isLogLevelEnabled (LogLevelError))
	{
		char txStr[10];
		char rxStr[10];
		pixi_hexEncode (tx, sizeof(tx), txStr, sizeof(txStr), ' ', "");
		pixi_hexEncode (rx, sizeof(rx), rxStr, sizeof(rxStr), ' ', "");
		LIBPIXI_LOG_DEBUG("pixi_adcRead result=%d, tx=%s rx=%s value=0x%08x value=%d", result, txStr, rxStr, value, value);
	}
	return value;
}

static int adcReadADC128S022 (uint adcChannel)
{
	// The protocol is 2-byte sequences for the request, but the
	// response is received in the following 2-byte sequence.
	// So you can read eight channels in using a 10-byte read/write
	// but you need 4 bytes to read a single channel.
	// TODO: provide an API for efficient reading of multiple channels.
	uint8 tx[4] = {
		adcChannel << 3,
		0,
		0,
		0
	};
	uint8 rx[4] = {0,0,0,0};
	int result = pixi_spiReadWrite (&adcSpi, tx, rx, sizeof (tx));
	if (result < 0)
		return result;

	uint value = makeUint12 (rx[2], rx[3]);
	if (pixi_isLogLevelEnabled (LogLevelError))
	{
		char txStr[20];
		char rxStr[20];
		pixi_hexEncode (tx, sizeof(tx), txStr, sizeof(txStr), ' ', "");
		pixi_hexEncode (rx, sizeof(rx), rxStr, sizeof(rxStr), ' ', "");
		LIBPIXI_LOG_DEBUG("pixi_adcRead result=%d, tx=%s rx=%s value=0x%08x value=%d", result, txStr, rxStr, value, value);
	}
	return value;
}

int pixi_adcRead (uint adcChannel)
{
	if (adcChannel >= adcChannels)
	{
		LIBPIXI_LOG_ERROR("ADC channel number %u is not less than %u", adcChannel, adcChannels);
		return -EINVAL;
	}

	int result = adcReadImpl (adcChannel);
	// Apparently this is useful for bringing CS down.
	char c;
	read (adcSpi.fd, &c, 0);

	if (result < 0)
	{
		LIBPIXI_ERROR(-result, "Error reading ADC channel");
		return result;
	}
	return result;
}
