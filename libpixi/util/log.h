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

#ifndef libpixi_util_log_h__included
#define libpixi_util_log_h__included

#include <libpixi/common.h>
#include <errno.h>

LIBPIXI_BEGIN_DECLS

///@defgroup util_log libpixi logging interface
/// The logging macros should be used to provide controllable error, warning
/// and other messages, while ordinary program output should use @c printf.
///
/// There are two sets of logging macros. Those with the @c LIBPIXI_ prefix
/// are intended for use in libpixi code.  Those with the APP_ prefix are
/// intended for application code. The macros should be sufficient for all
/// logging use and the @c _LOG_DEBUG, @c _LOG_INFO, @c _LOG_WARN and @c
/// _LOG_ERROR variants are the most extensively used. The macros work such
/// that if the current log level inhibits the message, no other work will have
/// been done except for testing the log level.
///
/// @c LIBPIXI_LOG_LEVEL environment variable controls how much of the
/// libpixi logging that you actually see, i.e. set @c LIBPIXI_LOG_LEVEL=warn
/// to see only warnings, errors and fatal errors, or
/// @c LIBPIXI_LOG_LEVEL=debug to also see debug and info logging.
///
/// The @c APP_ prefix macros inherit the log level defined by
/// @c LIBPIXI_LOG_LEVEL, but that can be overridden by an environment
/// variable @c <uppercase-app-name>_LOG_LEVEL. So if your application is
/// called @c pixi-ext, @c PIXI_EXT_LOG_LEVEL=error will set the log level of the
/// @c APP_ macros.
///
/// Setting the environment variable @c LIBPIXI_LOG_CODE_CONTEXT=yes will
/// cause each log line to be prefixed with source file and line number.
///
/// The @c LIBPIXI_LOG_FILE environment variable can be used to enable logging
/// to file, e.g. @c LIBPIXI_LOG_FILE=/tmp/pt.log. Log file lines additionally
/// have a date-time prefix.
///
///@{

typedef enum LogLevel
{
	LogLevelAll   = - 0x100000,

	LogLevelTrace =     0x1000,
	LogLevelDebug =     0x2000,
	LogLevelInfo  =     0x3000,
	LogLevelWarn  =     0x4000,
	LogLevelError =     0x5000,
	LogLevelFatal =     0x6000,

	LogLevelOff   =   0x100000
} LogLevel;

extern LogLevel pixi_logLevel;
extern bool     pixi_logColors;
extern bool     pixi_logFileContext;
/// If stdout is colour capable terminal, this has
/// the escape sequence to enable bold text, otherwise
/// it is an empty string.
extern const char* pixi_stdoutBold;
extern const char* pixi_stdoutReset;

typedef struct LogContext
{
	LogLevel     level;
	const char*  file;
	int          line;
	intptr       _reserved[2];
} LogContext;

LogLevel pixi_strToLogLevel (const char* levelStr, LogLevel defaultLevel);
const char* pixi_logLevelToStr (LogLevel level);

void pixi_logPrint (const LogContext* context, const char* format, ...) LIBPIXI_PRINTF_ARG(2);
void pixi_logError (const LogContext* context, int errnum, const char* format, ...) LIBPIXI_PRINTF_ARG(3);

/// Check if logging at @a level should be output.
static inline bool pixi_isLogLevelEnabled (LogLevel level) {
	return level >= pixi_logLevel;
}

/// Logs a format string to stderr. example:
/// <pre>LIBPIXI_GENERAL_LOG(pixi_logLevel, LogLevelError, "Unexpected value for gpio file %s: %s", fname, buf);</pre>
/// Don't use this directly, use one of the wrapper macros like LIBPIXI_LOG_ERROR.
#define LIBPIXI_GENERAL_LOG(confLevel, entryLevel, ...) \
	do {if ((entryLevel) >= confLevel) {\
		LogContext context = {entryLevel, __FILE__, __LINE__, {0, 0}};\
		pixi_logPrint (&context, __VA_ARGS__);\
	}} while (0)
/// Like LIBPIXI_GENERAL_LOG, but adds a suffix along the lines of perror(): (": %s", strerror(errnum)). example:
/// <pre>LIBPIXI_GENERAL_STRERROR_LOG(pixi_logLevel, LogLevelError, errno, "Could not open file %", fname);</pre>
/// Don't use this directly, use one of the wrapper macros like LIBPIXI_ERROR.
#define LIBPIXI_GENERAL_STRERROR_LOG(confLevel, entryLevel, errnum, ...) \
	do {if ((entryLevel) >= confLevel) {\
		LogContext context = {entryLevel, __FILE__, __LINE__, {0, 0}};\
		pixi_logError (&context, errnum, __VA_ARGS__);\
	}} while (0)

/// Wraps LIBPIXI_GENERAL_LOG with confLevel=pixi_logLevel
#define LIBPIXI_LOG(         level,         ...) LIBPIXI_GENERAL_LOG          (pixi_logLevel, level,         __VA_ARGS__)
/// Wraps LIBPIXI_GENERAL_STRERROR_LOG with confLevel=pixi_logLevel
#define LIBPIXI_STRERROR_LOG(level, errnum, ...) LIBPIXI_GENERAL_STRERROR_LOG (pixi_logLevel, level, errnum, __VA_ARGS__)

/// Wraps LIBPIXI_LOG with @c level=LogLevelTrace
#define LIBPIXI_LOG_TRACE(...) LIBPIXI_LOG(LogLevelTrace  , __VA_ARGS__)
#define LIBPIXI_LOG_DEBUG(...) LIBPIXI_LOG(LogLevelDebug  , __VA_ARGS__)
#define LIBPIXI_LOG_INFO( ...) LIBPIXI_LOG(LogLevelInfo   , __VA_ARGS__)
#define LIBPIXI_LOG_WARN( ...) LIBPIXI_LOG(LogLevelWarn   , __VA_ARGS__)
#define LIBPIXI_LOG_ERROR(...) LIBPIXI_LOG(LogLevelError  , __VA_ARGS__)
#define LIBPIXI_LOG_FATAL(...) LIBPIXI_LOG(LogLevelFatal  , __VA_ARGS__)

/// Wraps LIBPIXI_STRERROR_LOG with @c level=LogLevelTrace
#define LIBPIXI_ERROR_TRACE(errnum, ...) LIBPIXI_STRERROR_LOG(LogLevelTrace, errnum, __VA_ARGS__)
#define LIBPIXI_ERROR_DEBUG(errnum, ...) LIBPIXI_STRERROR_LOG(LogLevelDebug, errnum, __VA_ARGS__)
#define LIBPIXI_ERROR_INFO( errnum, ...) LIBPIXI_STRERROR_LOG(LogLevelInfo , errnum, __VA_ARGS__)
#define LIBPIXI_ERROR_WARN( errnum, ...) LIBPIXI_STRERROR_LOG(LogLevelWarn , errnum, __VA_ARGS__)
/// Wraps LIBPIXI_STRERROR_LOG with @c level=LogLevelError
#define LIBPIXI_ERROR(      errnum, ...) LIBPIXI_STRERROR_LOG(LogLevelError, errnum, __VA_ARGS__)
#define LIBPIXI_ERROR_FATAL(errnum, ...) LIBPIXI_STRERROR_LOG(LogLevelFatal, errnum, __VA_ARGS__)

/// Wraps LIBPIXI_STRERROR_LOG with @c level=LogLevelTrace, @c errnum=errno
#define LIBPIXI_ERRNO_TRACE( ...) LIBPIXI_STRERROR_LOG(LogLevelTrace, errno, __VA_ARGS__)
#define LIBPIXI_ERRNO_DEBUG( ...) LIBPIXI_STRERROR_LOG(LogLevelDebug, errno, __VA_ARGS__)
#define LIBPIXI_ERRNO_INFO(  ...) LIBPIXI_STRERROR_LOG(LogLevelInfo , errno, __VA_ARGS__)
#define LIBPIXI_ERRNO_WARN(  ...) LIBPIXI_STRERROR_LOG(LogLevelWarn , errno, __VA_ARGS__)
#define LIBPIXI_ERRNO_ERROR( ...) LIBPIXI_STRERROR_LOG(LogLevelError, errno, __VA_ARGS__)
#define LIBPIXI_ERRNO_FATAL( ...) LIBPIXI_STRERROR_LOG(LogLevelFatal, errno, __VA_ARGS__)

#define LIBPIXI_PRECONDITION_FAILURE(message) LIBPIXI_LOG_ERROR("%s(): precondition failure: " message, __PRETTY_FUNCTION__)

/// Test @c condition. If @c condition is false, log an error and return -EINVAL.
#define LIBPIXI_PRECONDITION(condition) \
	do if (LIBPIXI_UNLIKELY(!(condition))) { \
		LIBPIXI_PRECONDITION_FAILURE(#condition); \
		return -EINVAL; \
	} while (0)

/// Test @c condition. If @c condition is false, log an error message and return -EINVAL.
#define LIBPIXI_PRECONDITION_MSG(condition, message) \
	do if (LIBPIXI_UNLIKELY(!(condition))) { \
		LIBPIXI_PRECONDITION_FAILURE(#condition ": " message); \
		return -EINVAL; \
	} while (0)

/// Test @c pointer. If @c pointer is NULL, log an error and return -EINVAL.
#define LIBPIXI_PRECONDITION_NOT_NULL(pointer) \
	do if (LIBPIXI_UNLIKELY(pointer == NULL)) { \
		LIBPIXI_PRECONDITION_FAILURE(#pointer " is NULL"); \
		return -EINVAL; \
	} while (0)

///@} addtogroup

LIBPIXI_END_DECLS

#endif // !defined libpixi_util_log_h__included
