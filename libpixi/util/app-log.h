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

#ifndef libpixi_util_app_log_h__included
#define libpixi_util_app_log_h__included

#include <libpixi/util/log.h>

LIBPIXI_BEGIN_DECLS

///@addtogroup util_log
///@{

extern LogLevel pixi_appLogLevel;

/// Check if logging at @a level should be output.
static inline bool pixi_isAppLogLevelEnabled (LogLevel level) {
	return level >= pixi_appLogLevel;
}

/// Wraps LIBPIXI_GENERAL_LOG with confLevel=pixi_appLogLevel
#define APP_LOG(         level,         ...) LIBPIXI_GENERAL_LOG          (pixi_appLogLevel, level,         __VA_ARGS__)
/// Wraps LIBPIXI_GENERAL_STRERROR_LOG with confLevel=pixi_appLogLevel
#define APP_STRERROR_LOG(level, errnum, ...) LIBPIXI_GENERAL_STRERROR_LOG (pixi_appLogLevel, level, errnum, __VA_ARGS__)

/// Wraps APP_LOG with @c level=LogLevelTrace
#define APP_LOG_TRACE(...) APP_LOG(LogLevelTrace  , __VA_ARGS__)
#define APP_LOG_DEBUG(...) APP_LOG(LogLevelDebug  , __VA_ARGS__)
#define APP_LOG_INFO( ...) APP_LOG(LogLevelInfo   , __VA_ARGS__)
#define APP_LOG_WARN( ...) APP_LOG(LogLevelWarn   , __VA_ARGS__)
#define APP_LOG_ERROR(...) APP_LOG(LogLevelError  , __VA_ARGS__)
#define APP_LOG_FATAL(...) APP_LOG(LogLevelFatal  , __VA_ARGS__)

/// Wraps APP_STRERROR_LOG with @c level=LogLevelTrace
#define APP_ERROR_TRACE(errnum, ...) APP_STRERROR_LOG(LogLevelTrace, errnum, __VA_ARGS__)
#define APP_ERROR_DEBUG(errnum, ...) APP_STRERROR_LOG(LogLevelDebug, errnum, __VA_ARGS__)
#define APP_ERROR_INFO( errnum, ...) APP_STRERROR_LOG(LogLevelInfo , errnum, __VA_ARGS__)
#define APP_ERROR_WARN( errnum, ...) APP_STRERROR_LOG(LogLevelWarn , errnum, __VA_ARGS__)
/// Wraps APP_STRERROR_LOG with @c level=LogLevelError
#define APP_ERROR(      errnum, ...) APP_STRERROR_LOG(LogLevelError, errnum, __VA_ARGS__)
#define APP_ERROR_FATAL(errnum, ...) APP_STRERROR_LOG(LogLevelFatal, errnum, __VA_ARGS__)

/// Wraps APP_STRERROR_LOG with @c level=LogLevelTrace, @c errnum=errno
#define APP_ERRNO_TRACE( ...) APP_STRERROR_LOG(LogLevelTrace, errno, __VA_ARGS__)
#define APP_ERRNO_DEBUG( ...) APP_STRERROR_LOG(LogLevelDebug, errno, __VA_ARGS__)
#define APP_ERRNO_INFO(  ...) APP_STRERROR_LOG(LogLevelInfo , errno, __VA_ARGS__)
#define APP_ERRNO_WARN(  ...) APP_STRERROR_LOG(LogLevelWarn , errno, __VA_ARGS__)
#define APP_ERRNO_ERROR( ...) APP_STRERROR_LOG(LogLevelError, errno, __VA_ARGS__)
#define APP_ERRNO_FATAL( ...) APP_STRERROR_LOG(LogLevelFatal, errno, __VA_ARGS__)

#define APP_PRECONDITION_FAILURE(message) APP_LOG_ERROR("%s(): precondition failure: " message, LIBPIXI_FUNCTION)

/// Test @c condition. If @c condition is false, log an error and return -EINVAL.
#define APP_PRECONDITION(condition) \
	if (!(condition)) { \
		APP_PRECONDITION_FAILURE(#condition); \
		return -EINVAL; \
	}

/// Test @c pointer. If @c condition is NULL, log an error and return -EINVAL.
#define APP_PRECONDITION_NOT_NULL(pointer) \
	if (!(pointer)) { \
		APP_PRECONDITION_FAILURE(#pointer " is NULL"); \
		return -EINVAL; \
	}

///@} defgroup

LIBPIXI_END_DECLS

#endif // !defined libpixi_util_app_log_h__included
