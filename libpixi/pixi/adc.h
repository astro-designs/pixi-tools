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

#ifndef libpixi_pixi_adc_h__included
#define libpixi_pixi_adc_h__included


#include <libpixi/common.h>
#include <limits.h>

LIBPIXI_BEGIN_DECLS

///@defgroup PiXiAdc PiXi ADC / MCP-3204 / ADC088S021 / ADC128S022 interface
///@{

enum
{
	PixiAdcMaxChannels = 8,
};

///	Open the Pi SPI channel to the PiXi ADC. When finished,
///	call pixi_closePixi().
///	@return 0 on success, or -errno on error
int pixi_adcOpen (void);

///	Close the Pi SPI channel to the PiXi ADC.
///	@return 0 on success, or -errno on error
int pixi_adcClose (void);

///	Read an ADC channel. Return value is 12 bit unsigned integer
///	@return >=0 on success, negative error code on error
int pixi_adcRead (uint adcChannel);

///@} defgroup

LIBPIXI_END_DECLS


#endif // !defined libpixi_pixi_adc_h__included
