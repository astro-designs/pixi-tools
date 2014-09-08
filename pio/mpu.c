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

#include <libpixi/pixi/mpu.h>
#include <libpixi/util/file.h>
#include <libpixi/util/string.h>
#include <stdio.h>
#include "common.h"
#include "log.h"


static int mpuInit (void)
{
	// FIXME: extend this, and move to libpixi
	return pixi_mpuWriteRegister (MpuPowerManagement1, 0);
}


static int mpuMonitorTemp (void)
{
	int result = pixi_mpuOpen();
	if (result < 0)
		return result;

	mpuInit();

	result = 0;
	while (true)
	{
		result = pixi_mpuReadRegister16 (MpuTemperatureHigh);
		if (result < 0)
			break;
		int16 raw = result;
		double temp = mpuTemperatureToDegrees (raw);
		printf ("\rhex=0x%04x dec=%6d temperature=%7.3f degrees C", (uint16) raw, raw, temp);
		fflush (stdout);

		usleep (100 * 1000);
	}
	printf ("\n");

	pixi_mpuClose();

	return result;
}


static int mpuMonitorTempFn (const Command* command, uint argc, char* argv[])
{
	LIBPIXI_UNUSED(argv);
	if (argc != 1)
		return commandUsageError (command);

	return mpuMonitorTemp();
}
static Command mpuMonitorTempCmd =
{
	.name        = "mpu-monitor-temp",
	.description = "Monitor MPU temperature",
	.usage       = "usage: %s",
	.function    = mpuMonitorTempFn
};


static int mpuMonitorMotion (void)
{
	int result = pixi_mpuOpen();
	if (result < 0)
		return result;

	mpuInit();

	int16 accel[3] = {0,0,0};
	int16 gyro[3] = {0,0,0};
	result = 0;
	while (true)
	{
		// TODO: maybe merge into single read
		result = pixi_mpuReadRegisters16 (MpuAccelXHigh, accel, 3);
		if (result < 0)
			break;
		result = pixi_mpuReadRegisters16 (MpuGyroXHigh, gyro, 3);
		if (result < 0)
			break;
		printf ("\raccel [%6d %6d %6d] gyro [%6d %6d %6d]",
			accel[0], accel[1], accel[2],
     			gyro[0], gyro[1], gyro[2]
		);
		fflush (stdout);

		usleep (100 * 1000);
	}
	printf ("\n");

	pixi_mpuClose();

	return result;
}


static int mpuMonitorMotionFn (const Command* command, uint argc, char* argv[])
{
	LIBPIXI_UNUSED(argv);
	if (argc != 1)
		return commandUsageError (command);

	return mpuMonitorMotion();
}
static Command mpuMonitorMotionCmd =
{
	.name        = "mpu-monitor-motion",
	.description = "Monitor MPU motion",
	.usage       = "usage: %s",
	.function    = mpuMonitorMotionFn
};


static const Command* commands[] =
{
	&mpuMonitorTempCmd,
	&mpuMonitorMotionCmd,
};


static CommandGroup pixiMpuGroup =
{
	.name      = "pixi-mpu",
	.count     = ARRAY_COUNT(commands),
	.commands  = commands,
	.nextGroup = NULL
};

static void PIO_CONSTRUCTOR (1080) initGroup (void)
{
	addCommandGroup (&pixiMpuGroup);
}
