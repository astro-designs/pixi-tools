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

#ifndef libpixi_util_log_h__included
#define libpixi_util_log_h__included

#include <libpixi/common.h>
#include <errno.h>

LIBPIXI_BEGIN_DECLS

///@defgroup util_log libpixi logging interface
///@{

typedef enum LogLevel
{
	LogLevelAll,

	LogLevelTrace,
	LogLevelDebug,
	LogLevelInfo,
	LogLevelWarn,
	LogLevelError,
	LogLevelFatal,

	LogLevelOff
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
	void*        reserved1;
	void*        reserved2;
} LogContext;

void pixi_logInit (LogLevel level);
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
		LogContext context = {.level = entryLevel	, .file = __FILE__, .line = __LINE__, .reserved1 = 0, .reserved2 = 0};\
		pixi_logPrint (&context, __VA_ARGS__);\
	}} while (0)
/// Like LIBPIXI_GENERAL_LOG, but adds a suffix along the lines of perror(): (": %s", strerror(errnum)). example:
/// <pre>LIBPIXI_GENERAL_STRERROR_LOG(pixi_logLevel, LogLevelError, errno, "Could not open file %", fname);</pre>
/// Don't use this directly, use one of the wrapper macros like LIBPIXI_ERROR.
#define LIBPIXI_GENERAL_STRERROR_LOG(confLevel, entryLevel, errnum, ...) \
	do {if ((entryLevel) >= confLevel) {\
		LogContext context = {.level = entryLevel, .file = __FILE__, .line = __LINE__, .reserved1 = 0, .reserved2 = 0};\
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

/// Test @c pointer. If @c pointer is NULL, log an error and return -EINVAL.
#define LIBPIXI_PRECONDITION_NOT_NULL(pointer) \
	do if (LIBPIXI_UNLIKELY(!(pointer))) { \
		LIBPIXI_PRECONDITION_FAILURE(#pointer " is NULL"); \
		return -EINVAL; \
	} while (0)

///@} defgroup

LIBPIXI_END_DECLS

#endif // !defined libpixi_util_log_h__included
