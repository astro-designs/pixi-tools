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

#include <libpixi/util/log.h>
#include <errno.h>
#include <stdio.h>
#include <stdarg.h>
#include <stdlib.h>
#include <string.h>
#include <strings.h>
#include <unistd.h>

LogLevel pixi_logLevel = LogLevelInfo;
bool     pixi_logColors = false;

static const char AnsiReset[]   = "\033[0m";
static const char AnsiBold[]    = "\033[1m";
static const char AnsiBoldRed[] = "\033[1;31m";
static const char AnsiRed[]     = "\033[0;31m";
static const char AnsiGreen[]   = "\033[0;33m";

static const char* levelColors[LogLevelOff+1];

static const char* getLevelColor (LogLevel level)
{
	if (!pixi_logColors)
		return NULL;
	if ((size_t) level >= ARRAY_COUNT(levelColors))
		return NULL;
	return levelColors[level];
}

static bool useAnsiColors (void)
{
	const char* term = getenv ("TERM");
	if (!term)
		return false;
	if (!term[0] || 0 == strcasecmp (term, "dumb"))
		return false;
	return isatty (STDERR_FILENO) > 0;
}

void pixi_logInit (LogLevel level)
{
	pixi_logLevel = level;

	// TODO: this check is a bit too crude:
	pixi_logColors = useAnsiColors();
	if (!pixi_logColors)
		return;
	levelColors[LogLevelWarn]  = AnsiGreen;
	levelColors[LogLevelError] = AnsiRed;
	levelColors[LogLevelFatal] = AnsiBoldRed;
	levelColors[LogLevelOff]   = AnsiReset;
}

LogLevel pixi_strToLogLevel (const char* levelStr, LogLevel defaultLevel)
{
	if (!levelStr || !levelStr[0])
		return defaultLevel;
	if (0 == strcasecmp (levelStr, "all"    )) return LogLevelAll;
	if (0 == strcasecmp (levelStr, "trace"  )) return LogLevelTrace;
	if (0 == strcasecmp (levelStr, "debug"  )) return LogLevelDebug;
	if (0 == strcasecmp (levelStr, "info"   )) return LogLevelInfo;
	if (0 == strcasecmp (levelStr, "warn"   )) return LogLevelWarn;
	if (0 == strcasecmp (levelStr, "error"  )) return LogLevelError;
	if (0 == strcasecmp (levelStr, "fatal"  )) return LogLevelFatal;
	if (0 == strcasecmp (levelStr, "off"    )) return LogLevelOff;
	return defaultLevel;
}

const char* pixi_logLevelToStr (LogLevel level)
{
	switch (level)
	{
	case LogLevelAll     : return "all";
	case LogLevelTrace   : return "trace";
	case LogLevelDebug   : return "debug";
	case LogLevelInfo    : return "info";
	case LogLevelWarn    : return "warn";
	case LogLevelError   : return "error";
	case LogLevelFatal   : return "fatal";
	case LogLevelOff     : // fall through:
	default              : return "off";
	}
}

static void prefixLog (LogLevel level)
{
	fprintf (stderr, "%s: %s: ", program_invocation_short_name, pixi_logLevelToStr (level));
}

void pixi_logPrint (LogLevel level, const char* format, ...)
{
	va_list args;
	va_start(args, format);

	// TODO: fix thread safety
	const char* color = getLevelColor (level);
	if (color)
		fputs (color, stderr);
	prefixLog (level);
	vfprintf (stderr, format, args);
	if (color)
		fputs (levelColors[LogLevelOff], stderr);
	fputs ("\n", stderr);

	va_end(args);
}

void pixi_logError (LogLevel level, int errnum, const char* format, ...)
{
	va_list args;
	va_start(args, format);

	// TODO: fix thread safety
	const char* color = getLevelColor (level);
	if (color)
		fputs (color, stderr);

	prefixLog (level);
	vfprintf (stderr, format, args);
	fprintf (stderr, ": %s", strerror (errnum));
	if (color)
		fputs (levelColors[LogLevelOff], stderr);
	fputs ("\n", stderr);

	va_end(args);
}
