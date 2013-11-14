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

#include <libpixi/pixi/pwm.h>
#include <libpixi/pixi/spi.h>
#include <libpixi/pixi/registers.h>
#include <libpixi/util/log.h>
#include <string.h>

int pixi_pwmSet (SpiDevice* device, uint pwm, uint dutyCycle)
{
	LIBPIXI_PRECONDITION_NOT_NULL(device);
	LIBPIXI_PRECONDITION_NOT_NULL(pwm < 8);
	LIBPIXI_PRECONDITION_NOT_NULL(dutyCycle < 1024);
	return pixi_pixiSpiWriteValue16 (device, 	Pixi_PWM0_control + pwm, dutyCycle);
}

int pixi_pwmSetPercent (SpiDevice* device, uint pwm, double dutyCycle)
{
	LIBPIXI_PRECONDITION_NOT_NULL(dutyCycle >= 0);
	LIBPIXI_PRECONDITION_NOT_NULL(dutyCycle <= 100);
	uint cycle = dutyCycle * 1023.0;
	return pixi_pwmSet (device, pwm, cycle);
}
