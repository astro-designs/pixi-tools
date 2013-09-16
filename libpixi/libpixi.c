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

#include <libpixi/libpixi.h>
#include <libpixi/util/log.h>
#include <libpixi/util/string.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>


#define SYS_GPIO "/sys/class/gpio"
#define LIBPIXI_VERSION "0.0"

static void LIBPIXI_CONSTRUCTOR (1) initLib (void)
{
	pixi_logInit (pixi_strToLogLevel (getenv ("LIBPIXI_LOG_LEVEL"), LogLevelInfo));
	LIBPIXI_LOG_TRACE("libpixi initialised");
}

int pixi_initLib (void)
{
	return pixi_getPiBoardVersion();
}

const char* pixi_getLibVersion (void)
{
	return LIBPIXI_VERSION;
}

static char boardRevision[256] = "[unknown]";
static int boardVersion = -EINVAL;

int pixi_getPiBoardVersion (void)
{
	if (boardVersion >= 0)
		return boardVersion;

	FILE* file = fopen ("/proc/cpuinfo", "r");
	if (!file)
	{
		int err = errno;
		LIBPIXI_ERROR_FATAL(err, "Cannot open /proc/cpuinfo");
		return -err;
	}
	bool foundHardware = false;
	bool foundRevision = false;
	char buf[1024];
	while (fgets (buf, sizeof (buf), file))
	{
		Property prop;
		if (!pixi_strGetProperty (buf, ':', &prop))
			continue;

		if (0 == strcmp (prop.key, "Hardware"))
		{
			LIBPIXI_LOG_TRACE("Found hardware [%s]", prop.value);
			foundHardware = true;
			if (foundRevision)
				break;
			continue;
		}
		else if (0 == strcmp (prop.key, "Revision"))
		{
			LIBPIXI_LOG_TRACE("Found board revision [%s]", prop.value);
			pixi_strCopy (prop.value, boardRevision, ARRAY_COUNT(boardRevision));
			foundRevision = true;
			if (foundHardware)
				break;
		}
	}
	fclose (file);
	if (!foundHardware)
		LIBPIXI_LOG_FATAL("Did not find 'Hardware' in /proc/cpuinfo");
	if (!foundRevision)
		LIBPIXI_LOG_FATAL("Did not find 'Revision' in /proc/cpuinfo");
	if (foundHardware && foundRevision)
	{
		if (pixi_strEndsWith (boardRevision, "0002") || pixi_strEndsWith (boardRevision, "0003"))
			boardVersion = 1;
		else
			boardVersion = 2;
		LIBPIXI_LOG_TRACE("Setting board version to %d", boardVersion);
	}
	return boardVersion;
}

const char* pixi_getPiBoardRevision (void)
{
	pixi_getPiBoardVersion();
	return boardRevision;
}
