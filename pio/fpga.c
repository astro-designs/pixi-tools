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

#include <libpixi/pixi/fpga.h>
#include <libpixi/pixi/spi.h>
#include "common.h"
#include "log.h"
#include <stdio.h>

static const char DefaultFpga[] = "/home/pixi-200/pixi.bin"; // TODO: belongs in ../share/pixi?

static int64 getVersion (void)
{
	int result = pixi_openPixi();
	if (result < 0)
	{
		PIO_ERROR(-result, "Error opening PiXi SPI connection");
		return result;
	}

	int64 version = pixi_fpgaGetVersion();
	pixi_closePixi();
	if (version < 0)
	{
		PIO_ERROR(-version, "Error getting FPGA version");
		return version;
	}
	if (version == 0)
	{
		PIO_LOG_ERROR("Error getting FPGA version: value is zero");
		return -EINVAL;
	}
	return version;
}

static int formatBuildTime (int64 version, char* buffer, size_t bufferLen)
{
	time_t time = pixi_fpgaVersionToTime (version);
	if (time < 0)
	{
		PIO_ERROR(-time, "Error converting FPGA version to time");
		return time;
	}
	// Use strftime %c to get localised time format
	size_t chars = strftime (buffer, bufferLen, "%c", gmtime (&time));
	if (chars <= 0)
	{
		PIO_ERROR(-time, "Error formatting time value of %ld", time);
		return -EINVAL;
	}
	return 0;
}

static int fpgaLoadFn (const Command* command, uint argc, char* argv[])
{
	if (argc > 2)
		return commandUsageError (command);

	const char* filename = NULL;
	if (argc > 1)
		filename = argv[1];
	else
		filename = DefaultFpga;
	int result = pixi_fpgaLoadFile(filename);
	if (result < 0)
	{
		PIO_ERROR(-result, "Could not load FPGA file from [%s]", filename);
		return result;
	}
	printf ("FPGA loaded\n");
	int64 version = getVersion();
	if (version < 0)
	{
		PIO_ERROR(-result, "Could not get loaded FPGA version");
		return result;
	}
	printf ("FPGA Version: %012llx\n", (ulonglong) version);
	char buildTime[80];
	result = formatBuildTime (version, buildTime, sizeof (buildTime));
	if (result < 0)
		return result;

	printf ("FPGA Build-time: %s\n", buildTime);
	return 0;
}
static Command fpgaLoadCmd =
{
	.name        = "load-fpga",
	.description = "load an FPGA image to the PiXi",
	.usage       = "usage: %s [FPGAFILE]",
	.function    = fpgaLoadFn
};

static int fpgaGetVersionFn (const Command* command, uint argc, char* argv[])
{
	LIBPIXI_UNUSED(argv);
	if (argc != 1)
		return commandUsageError (command);

	int64 version = getVersion();
	if (version < 0)
	{
		PIO_ERROR(-version, "Error getting FPGA version");
		return version;
	}
	printf ("%012llx\n", (ulonglong) version);
	return 0;
}
static Command fpgaGetVersionCmd =
{
	.name        = "fpga-version",
	.description = "print the PiXi FPGA version",
	.usage       = "usage: %s",
	.function    = fpgaGetVersionFn
};


static int fpgaGetBuildTimeFn (const Command* command, uint argc, char* argv[])
{
	LIBPIXI_UNUSED(argv);
	if (argc != 1)
		return commandUsageError (command);

	int64 version = getVersion();
	if (version == 0)
	{
		PIO_LOG_ERROR("Error getting FPGA version: value is zero");
		return -EINVAL;
	}
	if (version < 0)
	{
		PIO_ERROR(-version, "Error getting FPGA version");
		return version;
	}

	char buildTime[80];
	int result = formatBuildTime (version, buildTime, sizeof (buildTime));
	if (result < 0)
		return result;

	printf ("%s\n", buildTime);
	return 0;
}
static Command fpgaGetBuildTimeCmd =
{
	.name        = "fpga-build-time",
	.description = "print the PiXi FPGA build time",
	.usage       = "usage: %s",
	.function    = fpgaGetBuildTimeFn
};

static int fpgaGetIdFn (const Command* command, uint argc, char* argv[])
{
	LIBPIXI_UNUSED(argv);
	if (argc != 1)
		return commandUsageError (command);

	int result = pixi_openPixi();
	if (result < 0)
	{
		PIO_ERROR(-result, "Error opening PiXi SPI connection");
		return result;
	}

	int64 id = pixi_fpgaGetId();
	pixi_closePixi();
	if (id < 0)
	{
		PIO_ERROR(-id, "Error getting FPGA ID");
		return id;
	}
	if (id == 0)
	{
		PIO_LOG_ERROR("Error getting FPGA ID: value is zero");
		return -EINVAL;
	}

	printf ("%14llX\n", (ulonglong) id);
	return 0;
}
static Command fpgaGetIdCmd =
{
	.name        = "fpga-id",
	.description = "print the PiXi FPGA's Xilinx ID",
	.usage       = "usage: %s",
	.function    = fpgaGetIdFn
};


static const Command* commands[] =
{
	&fpgaLoadCmd,
	&fpgaGetVersionCmd,
	&fpgaGetBuildTimeCmd,
	&fpgaGetIdCmd,
};


static CommandGroup pixiFpgaGroup =
{
	.name      = "pixi-fpga",
	.count     = ARRAY_COUNT(commands),
	.commands  = commands,
	.nextGroup = NULL
};

static void PIO_CONSTRUCTOR (1010) initGroup (void)
{
	addCommandGroup (&pixiFpgaGroup);
}
