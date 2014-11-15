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
#include <libpixi/util/io.h>
#include <libpixi/util/string.h>
#include <stdio.h>
#include "common.h"
#include "log.h"


const int interval = 100 * 1000;

static int mpuInit (void)
{
	// FIXME: extend this, and move to libpixi
	return pixi_mpuWriteRegister (MpuPowerManagement1, 0);
}

static int mpuOpenInit (void)
{
	int result = pixi_mpuOpen();
	if (result < 0)
		return result;
	return mpuInit();
}

static int mpuMagOpenInit (void)
{
	int result = mpuOpenInit();
	if (result < 0)
		return result;
	result = pixi_mpuMagOpen();
	if (result < 0)
		pixi_mpuClose();
	return result;
}


static int mpuGetAccelScale (void)
{
	int result = mpuOpenInit();
	if (result < 0)
		return result;

	result = pixi_mpuGetAccelScale();
	if (result >= 0)
		printf ("%d\n", result);

	pixi_mpuClose();

	return result;
}
static int mpuGetAccelScaleFn (const Command* command, uint argc, char* argv[])
{
	LIBPIXI_UNUSED(argv);
	if (argc != 1)
		return commandUsageError (command);

	return mpuGetAccelScale();
}
static Command mpuGetAccelScaleCmd =
{
	.name        = "mpu-get-acc-scale",
	.description = "Get the accelerometer range (g)",
	.usage       = "usage: %s",
	.function    = mpuGetAccelScaleFn
};


static int mpuSetAccelScale (uint scale)
{
	int result = mpuOpenInit();
	if (result < 0)
		return result;

	result = pixi_mpuSetAccelScale (scale);

	pixi_mpuClose();

	return result;
}
static int mpuSetAccelScaleFn (const Command* command, uint argc, char* argv[])
{
	LIBPIXI_UNUSED(argv);
	if (argc != 2)
		return commandUsageError (command);

	uint scale = pixi_parseLong (argv[1]);

	return mpuSetAccelScale (scale);
}
static Command mpuSetAccelScaleCmd =
{
	.name        = "mpu-set-acc-scale",
	.description = "Set the accelerometer range (g)",
	.usage       = "usage: %s 2|4|8|16",
	.function    = mpuSetAccelScaleFn
};


static int mpuGetGyroScale (void)
{
	int result = mpuOpenInit();
	if (result < 0)
		return result;

	result = pixi_mpuGetGyroScale();
	if (result >= 0)
		printf ("%d\n", result);

	pixi_mpuClose();

	return result;
}
static int mpuGetGyroScaleFn (const Command* command, uint argc, char* argv[])
{
	LIBPIXI_UNUSED(argv);
	if (argc != 1)
		return commandUsageError (command);

	return mpuGetGyroScale();
}
static Command mpuGetGyroScaleCmd =
{
	.name        = "mpu-get-gyro-scale",
	.description = "Get the gyroscope range (degrees/sec)",
	.usage       = "usage: %s",
	.function    = mpuGetGyroScaleFn
};


static int mpuSetGyroScale (uint scale)
{
	int result = mpuOpenInit();
	if (result < 0)
		return result;

	result = pixi_mpuSetGyroScale (scale);

	pixi_mpuClose();

	return result;
}
static int mpuSetGyroScaleFn (const Command* command, uint argc, char* argv[])
{
	LIBPIXI_UNUSED(argv);
	if (argc != 2)
		return commandUsageError (command);

	uint scale = pixi_parseLong (argv[1]);

	return mpuSetGyroScale (scale);
}
static Command mpuSetGyroScaleCmd =
{
	.name        = "mpu-set-gyro-scale",
	.description = "Set the gyroscope range (degrees/sec)",
	.usage       = "usage: %s 250|500|1000|2000",
	.function    = mpuSetGyroScaleFn
};


static int mpuMonitorTemp (void)
{
	int result = mpuOpenInit();
	if (result < 0)
		return result;

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

		usleep (interval);
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


static int mpuReadTemp (void)
{
	int result = mpuOpenInit();
	if (result < 0)
		return result;

	result = pixi_mpuReadRegister16 (MpuTemperatureHigh);
	if (result < 0)
		return result;

	int16 raw = result;
	double temp = mpuTemperatureToDegrees (raw);
	printf ("%.3f\n", temp);

	pixi_mpuClose();

	return result;
}

static int mpuReadTempFn (const Command* command, uint argc, char* argv[])
{
	LIBPIXI_UNUSED(argv);
	if (argc != 1)
		return commandUsageError (command);

	return mpuReadTemp();
}
static Command mpuReadTempCmd =
{
	.name        = "mpu-read-temp",
	.description = "Read MPU temperature (degrees C)",
	.usage       = "usage: %s",
	.function    = mpuReadTempFn
};


static int mpuMonitorMotion (void)
{
	int result = mpuOpenInit();
	if (result < 0)
		return result;

	result = pixi_mpuGetAccelScale();
	if (result < 0)
	{
		pixi_mpuClose();
		return result;
	}
	const double accelScale = result / 32768.0;

	result = pixi_mpuGetGyroScale();
	if (result < 0)
	{
		pixi_mpuClose();
		return result;
	}
	const double gyroScale = result / 32768.0;

	MpuMotion motion;
	result = 0;
	while (true)
	{
		result = pixi_mpuReadMotion (&motion);
		if (result < 0)
			break;
		printf ("\r[%8.4f %8.4f %8.4f]g [%8.3f %8.3f %8.3f]dps",
			accelScale * motion.accel.x, accelScale * motion.accel.y, accelScale * motion.accel.z,
			gyroScale * motion.gyro.x, gyroScale * motion.gyro.y, gyroScale * motion.gyro.z
		);
		fflush (stdout);

		usleep (interval);
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


static int mpuMonitorMag (void)
{
	int result = mpuMagOpenInit();
	if (result < 0)
		return result;

	result = 0;
	MpuAxes adjust = {0,0,0};
	result = pixi_mpuReadMagAdjust (&adjust);
	if (result < 0)
		APP_ERROR(-result, "Failed to read magnetometer adjustment values");

	double x = mpuMagGetScale (adjust.x);
	double y = mpuMagGetScale (adjust.y);
	double z = mpuMagGetScale (adjust.z);

	const char* format = "\r[%8.3f %8.3f %8.3f]uT";
	if (pixi_isLocaleEncodingUtf8())
		format = "\r[%8.3f %8.3f %8.3f]μT";

	while (true)
	{
		MpuAxes axes;
		result = pixi_mpuReadMag (&axes);
		if (result < 0)
			break;
		printf (format, x * axes.x, y * axes.y, z * axes.z);
		fflush (stdout);

		usleep (interval);
	}
	printf ("\n");

	pixi_mpuClose();

	return result;
}


static int mpuMonitorMagFn (const Command* command, uint argc, char* argv[])
{
	LIBPIXI_UNUSED(argv);
	if (argc != 1)
		return commandUsageError (command);

	return mpuMonitorMag();
}
static Command mpuMonitorMagCmd =
{
	.name        = "mpu-monitor-mag",
	.description = "Monitor MPU magnetometer",
	.usage       = "usage: %s",
	.function    = mpuMonitorMagFn
};


static int mpuReadAcc (void)
{
	int result = mpuOpenInit();
	if (result < 0)
		return result;

	result = pixi_mpuGetAccelScale();
	if (result < 0)
	{
		pixi_mpuClose();
		return result;
	}
	const double scale = result / 32768.0;

	MpuAxes ax;
	result = pixi_mpuReadAccel (&ax);
	if (result >= 0)
		printf ("%.4f %.4f %.4f\n", scale * ax.x, scale * ax.y, scale * ax.z);

	pixi_mpuClose();

	return result;
}


static int mpuReadAccFn (const Command* command, uint argc, char* argv[])
{
	LIBPIXI_UNUSED(argv);
	if (argc != 1)
		return commandUsageError (command);

	return mpuReadAcc();
}
static Command mpuReadAccCmd =
{
	.name        = "mpu-read-acc",
	.description = "Read accelerometer (g)",
	.usage       = "usage: %s",
	.function    = mpuReadAccFn
};


static int mpuReadAccX (uint address, const Command* command, uint argc, char* argv[])
{
	LIBPIXI_UNUSED(argv);
	if (argc != 1)
		return commandUsageError (command);

	int result = mpuOpenInit();
	if (result < 0)
		return result;

	result = pixi_mpuGetAccelScale();
	if (result < 0)
	{
		pixi_mpuClose();
		return result;
	}
	const double scale = result / 32768.0;

	result = pixi_mpuReadRegister16 (address);
	if (result >= 0)
		printf ("%.4f\n", scale * (int16) result);

	pixi_mpuClose();

	return result;
}


static int mpuReadAccXFn (const Command* command, uint argc, char* argv[])
{
	return mpuReadAccX (MpuAccelXHigh, command, argc, argv);
}
static Command mpuReadAccXCmd =
{
	.name        = "mpu-read-acc-x",
	.description = "Read accelerometer x value (g)",
	.usage       = "usage: %s",
	.function    = mpuReadAccXFn
};


static int mpuReadAccYFn (const Command* command, uint argc, char* argv[])
{
	return mpuReadAccX (MpuAccelYHigh, command, argc, argv);
}
static Command mpuReadAccYCmd =
{
	.name        = "mpu-read-acc-y",
	.description = "Read accelerometer y value (g)",
	.usage       = "usage: %s",
	.function    = mpuReadAccYFn
};


static int mpuReadAccZFn (const Command* command, uint argc, char* argv[])
{
	return mpuReadAccX (MpuAccelZHigh, command, argc, argv);
}
static Command mpuReadAccZCmd =
{
	.name        = "mpu-read-acc-z",
	.description = "Read accelerometer z value (g)",
	.usage       = "usage: %s",
	.function    = mpuReadAccZFn
};


static int mpuReadGyro (void)
{
	int result = mpuOpenInit();
	if (result < 0)
		return result;

	result = pixi_mpuGetGyroScale();
	if (result < 0)
	{
		pixi_mpuClose();
		return result;
	}
	const double scale = result / 32768.0;

	MpuAxes ax;
	result = pixi_mpuReadGyro (&ax);
	if (result >= 0)
		printf ("%.3f %.3f %.3f\n", scale * ax.x, scale * ax.y, scale * ax.z);

	pixi_mpuClose();

	return result;
}


static int mpuReadGyroFn (const Command* command, uint argc, char* argv[])
{
	LIBPIXI_UNUSED(argv);
	if (argc != 1)
		return commandUsageError (command);

	return mpuReadGyro();
}
static Command mpuReadGyroCmd =
{
	.name        = "mpu-read-gyro",
	.description = "Read gyroscope (dps)",
	.usage       = "usage: %s",
	.function    = mpuReadGyroFn
};


static int mpuReadGyroX (uint address, const Command* command, uint argc, char* argv[])
{
	LIBPIXI_UNUSED(argv);
	if (argc != 1)
		return commandUsageError (command);

	int result = mpuOpenInit();
	if (result < 0)
		return result;

	result = pixi_mpuGetGyroScale();
	if (result < 0)
	{
		pixi_mpuClose();
		return result;
	}
	const double scale = result / 32768.0;

	result = pixi_mpuReadRegister16 (address);
	if (result >= 0)
		printf ("%.3f\n", scale * (int16) result);

	pixi_mpuClose();

	return result;
}


static int mpuReadGyroXFn (const Command* command, uint argc, char* argv[])
{
	return mpuReadGyroX (MpuGyroXHigh, command, argc, argv);
}
static Command mpuReadGyroXCmd =
{
	.name        = "mpu-read-gyro-x",
	.description = "Read gyroscope x value (dps)",
	.usage       = "usage: %s",
	.function    = mpuReadGyroXFn
};


static int mpuReadGyroYFn (const Command* command, uint argc, char* argv[])
{
	return mpuReadGyroX (MpuGyroYHigh, command, argc, argv);
}
static Command mpuReadGyroYCmd =
{
	.name        = "mpu-read-gyro-y",
	.description = "Read gyroscope y value (dps)",
	.usage       = "usage: %s",
	.function    = mpuReadGyroYFn
};


static int mpuReadGyroZFn (const Command* command, uint argc, char* argv[])
{
	return mpuReadGyroX (MpuGyroZHigh, command, argc, argv);
}
static Command mpuReadGyroZCmd =
{
	.name        = "mpu-read-gyro-z",
	.description = "Read gyroscope z value (dps)",
	.usage       = "usage: %s",
	.function    = mpuReadGyroZFn
};


static int mpuReadMagAdjusted (double values[])
{
	int result = mpuMagOpenInit();
	if (result < 0)
		return result;

	result = 0;
	MpuAxes adjust = {0,0,0};
	result = pixi_mpuReadMagAdjust (&adjust);
	if (result < 0)
		APP_ERROR(-result, "Failed to read magnetometer adjustment values");

	MpuAxes axes;
	result = pixi_mpuReadMag (&axes);
	if (result >= 0)
	{
		values[0] = mpuMagAdjust (adjust.x, axes.x);
		values[1] = mpuMagAdjust (adjust.y, axes.y);
		values[2] = mpuMagAdjust (adjust.z, axes.z);
	}
	else
		APP_ERROR(-result, "Failed to read magnetometer values");

	pixi_mpuClose();

	return result;
}

static int mpuReadMagFn (const Command* command, uint argc, char* argv[])
{
	LIBPIXI_UNUSED(argv);
	if (argc != 1)
		return commandUsageError (command);

	double axes[3];
	int result = mpuReadMagAdjusted (axes);
	if (result < 0)
		return result;

	printf ("%.3f %.3f %.3f\n", axes[0], axes[1], axes[2]);

	return result;
}
static Command mpuReadMagCmd =
{
	.name        = "mpu-read-mag",
	.description = "Read magnetometer",
	.usage       = "usage: %s",
	.function    = mpuReadMagFn
};

static int mpuReadMagAxis (uint axis, const Command* command, uint argc, char* argv[])
{
	LIBPIXI_UNUSED(argv);
	if (argc != 1)
		return commandUsageError (command);

	double axes[3];
	int result = mpuReadMagAdjusted (axes);
	if (result < 0)
		return result;

	printf ("%.3f\n", axes[axis]);

	return result;
}


static int mpuReadMagXFn (const Command* command, uint argc, char* argv[])
{
	return mpuReadMagAxis (0, command, argc, argv);
}
static Command mpuReadMagXCmd =
{
	.name        = "mpu-read-mag-x",
	.description = "Read magnetometer x value",
	.usage       = "usage: %s",
	.function    = mpuReadMagXFn
};


static int mpuReadMagYFn (const Command* command, uint argc, char* argv[])
{
	return mpuReadMagAxis (1, command, argc, argv);
}
static Command mpuReadMagYCmd =
{
	.name        = "mpu-read-mag-y",
	.description = "Read magnetometer y value ",
	.usage       = "usage: %s",
	.function    = mpuReadMagYFn
};


static int mpuReadMagZFn (const Command* command, uint argc, char* argv[])
{
	return mpuReadMagAxis (2, command, argc, argv);
}
static Command mpuReadMagZCmd =
{
	.name        = "mpu-read-mag-z",
	.description = "Read magnetometer z value",
	.usage       = "usage: %s",
	.function    = mpuReadMagZFn
};


static int mpuMagId (void)
{
	int result = mpuMagOpenInit();
	if (result < 0)
		return result;

	result = pixi_mpuMagReadRegister (0);
	if (result < 0)
		return result;

	printf ("%02x\n", result);

	pixi_mpuClose();

	return result;
}

static int mpuMagIdFn (const Command* command, uint argc, char* argv[])
{
	LIBPIXI_UNUSED(argv);
	if (argc != 1)
		return commandUsageError (command);

	return mpuMagId();
}
static Command mpuMagIdCmd =
{
	.name        = "mpu-read-mag-id",
	.description = "Read MPU magnetometer ID",
	.usage       = "usage: %s",
	.function    = mpuMagIdFn
};


static const Command* commands[] =
{
	&mpuGetAccelScaleCmd,
	&mpuSetAccelScaleCmd,
	&mpuGetGyroScaleCmd,
	&mpuSetGyroScaleCmd,
	&mpuMonitorTempCmd,
	&mpuMonitorMotionCmd,
	&mpuMonitorMagCmd,
	&mpuReadTempCmd,
	&mpuReadAccCmd,
	&mpuReadAccXCmd,
	&mpuReadAccYCmd,
	&mpuReadAccZCmd,
	&mpuReadGyroCmd,
	&mpuReadGyroXCmd,
	&mpuReadGyroYCmd,
	&mpuReadGyroZCmd,
	&mpuReadMagCmd,
	&mpuReadMagXCmd,
	&mpuReadMagYCmd,
	&mpuReadMagZCmd,
	&mpuMagIdCmd,
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
