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
#include <libpixi/util/string.h>
#include <ctype.h>
#include <errno.h>
#include <stdio.h>
#include <stdarg.h>
#include <stdlib.h>
#include <string.h>
#include <strings.h>
#include <unistd.h>
#include "../private.h"

LogLevel pixi_logLevel = LogLevelInfo;
LogLevel pixi_appLogLevel = LogLevelInfo;
bool     pixi_logColors = false;
bool     pixi_logFileContext = false;

const char* pixi_stdoutBold  = "";
const char* pixi_stdoutReset = "";

static const char AnsiReset[]   = "\033[0m";
static const char AnsiBold[]    = "\033[1m";
static const char AnsiBoldRed[] = "\033[1;31m";
static const char AnsiRed[]     = "\033[0;31m";
static const char AnsiGreen[]   = "\033[0;33m";

static const char* levelColors[LogLevelOff+1];

static const long  maxLogSize   = 10 * 1024 * 1024;
static const char* logFilename  = NULL;
static FILE*       logFile      = NULL;
static long        logSize      =  0;

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
	// TODO: this test is too crude
	const char* term = getenv ("TERM");
	if (!term)
		return false;
	if (!term[0] || 0 == strcasecmp (term, "dumb"))
		return false;
	bool result = isatty (STDERR_FILENO) > 0;
	if (isatty (STDOUT_FILENO))
	{
		pixi_stdoutBold  = AnsiBold;
		pixi_stdoutReset = AnsiReset;
	}
	return result;
}

static void openLogFile (void)
{
	// TODO: locking for multi-threading
	if (logFile)
		fclose (logFile);
	logFile = fopen (logFilename, "a");
	if (!logFile)
	{
		LIBPIXI_ERROR(errno, "Failed to open log file %s", logFilename);
		return;
	}
	logSize = ftell (logFile);
}

static void rotateLogFile (void)
{
	char oldName[1024];
	int count = snprintf (oldName, sizeof (oldName), "%s.1", logFilename);
	if (count >= (int) sizeof (oldName))
	{
		perror ("Error formatting log rotation filename");
		return;
	}
	int result = rename (logFilename, oldName);
	if (result < 0)
	{
		perror ("Error performing log rotation rename");
		return;
	}
	openLogFile();
}


static const char* getAppLogLevelEnv (void)
{
	const char* prog = program_invocation_short_name;
	if (!prog)
		return NULL;

	// get the value of environment var {uppercase(app-name)}_LOG_LEVEL
	const char* suffix = "_LOG_LEVEL";
	uint progLen = strlen (prog);
	char label[progLen + strlen (suffix) + 1];
	uint i = 0;
	for ( ; i < progLen; i++)
	{
		if (isalnum (prog[i]))
			label[i] = toupper (prog[i]);
		else
			label[i] = '_';
	}
	strcpy (label+i, suffix);
	return getenv (label);
}

void pixi_logInit (void)
{
	pixi_logLevel = pixi_strToLogLevel (getenv ("LIBPIXI_LOG_LEVEL"), LogLevelInfo);
	// app inherit libpixi level
	pixi_appLogLevel = pixi_strToLogLevel (getAppLogLevelEnv(), pixi_logLevel);

	// TODO: this check is a bit too crude:
	pixi_logColors = useAnsiColors();
	if (pixi_logColors)
	{
		levelColors[LogLevelWarn]  = AnsiGreen;
		levelColors[LogLevelError] = AnsiRed;
		levelColors[LogLevelFatal] = AnsiBoldRed;
		levelColors[LogLevelOff]   = AnsiReset;
	}
	const char* ctx = getenv ("LIBPIXI_LOG_CODE_CONTEXT");
	pixi_logFileContext = (ctx && 0 == strcasecmp (ctx, "yes"));

	logFilename = getenv ("LIBPIXI_LOG_FILE");
	if (logFilename)
		openLogFile();
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

static void logVPrintf (const LogContext* context, const char* errorStr, const char* format, va_list formatArgs)
{
	char buffer[2048] = "";
	int count = vsnprintf (buffer, sizeof (buffer), format, formatArgs);
	if (count >= (int) sizeof (buffer))
		fprintf (stderr, "Error: log entry is too long for format string [%s]\n", format);

	const char* file = "";
	char line[18] = "";
	if (pixi_logFileContext && context->file)
	{
		file = context->file;
		snprintf (line, sizeof (line), ":%d: ", context->line);
	}

	const char* prog = program_invocation_short_name;
	const char* lev  = pixi_logLevelToStr (context->level);
	const char* startColor = getLevelColor (context->level);
	const char* endColor   = "";
	if (startColor)
		endColor = levelColors[LogLevelOff];
	else
		startColor = "";
	const char* errorColon = "";
	if (errorStr)
		errorColon = ": ";
	else
		errorStr = "";

	fprintf (stderr, "%s%s%s%s: %s: %s%s%s%s\n",
		file,
		line,
		startColor,
		prog,
		lev,
		buffer,
		errorColon,
		errorStr,
		endColor
		);
	if (logFile)
	{
		if (logSize > maxLogSize)
			rotateLogFile();
		char timeStr[40] = "";
		pixi_formatCurTime (timeStr, sizeof (timeStr));
		int written = fprintf (logFile, "%s %s%s%s: %s%s%s\n",
			timeStr,
			file,
			line,
			lev,
			buffer,
			errorColon,
			errorStr
			);
		if (written < 0)
			perror ("Error writing to log file");
		else
		{
			fflush (logFile);
			logSize += written;
		}
	}
}

void pixi_logPrint (const LogContext* context, const char* format, ...)
{
	va_list args;
	va_start(args, format);

	logVPrintf (context, NULL, format, args);

	va_end(args);
}

void pixi_logError (const LogContext* context, int errnum, const char* format, ...)
{
	va_list args;
	va_start(args, format);

	char buffer[256];
	char* errorStr = strerror_r (errnum, buffer, sizeof (buffer));

	logVPrintf (context, errorStr, format, args);

	va_end(args);
}
