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

#ifndef pio_log_h__included
#define pio_log_h__included


#include <libpixi/util/log.h>

extern LogLevel pio_logLevel;

/// Check if logging at @a level should be output.
static inline bool pio_isLogLevelEnabled (LogLevel level) {
	return level >= pio_logLevel;
}

/// Wraps LIBPIXI_GENERAL_LOG with confLevel=pio_logLevel
#define PIO_LOG(         level,         ...) LIBPIXI_GENERAL_LOG          (pio_logLevel, level,         __VA_ARGS__)
/// Wraps LIBPIXI_GENERAL_STRERROR_LOG with confLevel=pio_logLevel
#define PIO_STRERROR_LOG(level, errnum, ...) LIBPIXI_GENERAL_STRERROR_LOG (pio_logLevel, level, errnum, __VA_ARGS__)

/// Wraps PIO_LOG with @c level=LogLevelTrace
#define PIO_LOG_TRACE(...) PIO_LOG(LogLevelTrace  , __VA_ARGS__)
#define PIO_LOG_DEBUG(...) PIO_LOG(LogLevelDebug  , __VA_ARGS__)
#define PIO_LOG_INFO( ...) PIO_LOG(LogLevelInfo   , __VA_ARGS__)
#define PIO_LOG_WARN( ...) PIO_LOG(LogLevelWarn   , __VA_ARGS__)
#define PIO_LOG_ERROR(...) PIO_LOG(LogLevelError  , __VA_ARGS__)
#define PIO_LOG_FATAL(...) PIO_LOG(LogLevelFatal  , __VA_ARGS__)

/// Wraps PIO_STRERROR_LOG with @c level=LogLevelTrace
#define PIO_ERROR_TRACE(errnum, ...) PIO_STRERROR_LOG(LogLevelTrace, errnum, __VA_ARGS__)
#define PIO_ERROR_DEBUG(errnum, ...) PIO_STRERROR_LOG(LogLevelDebug, errnum, __VA_ARGS__)
#define PIO_ERROR_INFO( errnum, ...) PIO_STRERROR_LOG(LogLevelInfo , errnum, __VA_ARGS__)
#define PIO_ERROR_WARN( errnum, ...) PIO_STRERROR_LOG(LogLevelWarn , errnum, __VA_ARGS__)
/// Wraps PIO_STRERROR_LOG with @c level=LogLevelError
#define PIO_ERROR(      errnum, ...) PIO_STRERROR_LOG(LogLevelError, errnum, __VA_ARGS__)
#define PIO_ERROR_FATAL(errnum, ...) PIO_STRERROR_LOG(LogLevelFatal, errnum, __VA_ARGS__)

/// Wraps PIO_STRERROR_LOG with @c level=LogLevelTrace, @c errnum=errno
#define PIO_ERRNO_TRACE( ...) PIO_STRERROR_LOG(LogLevelTrace, errno, __VA_ARGS__)
#define PIO_ERRNO_DEBUG( ...) PIO_STRERROR_LOG(LogLevelDebug, errno, __VA_ARGS__)
#define PIO_ERRNO_INFO(  ...) PIO_STRERROR_LOG(LogLevelInfo , errno, __VA_ARGS__)
#define PIO_ERRNO_WARN(  ...) PIO_STRERROR_LOG(LogLevelWarn , errno, __VA_ARGS__)
#define PIO_ERRNO_ERROR( ...) PIO_STRERROR_LOG(LogLevelError, errno, __VA_ARGS__)
#define PIO_ERRNO_FATAL( ...) PIO_STRERROR_LOG(LogLevelFatal, errno, __VA_ARGS__)

#define PIO_PRECONDITION_FAILURE(message) PIO_LOG_ERROR("%s(): precondition failure: " message, __PRETTY_FUNCTION__)

/// Test @c condition. If @c condition is false, log an error and return -EINVAL.
#define PIO_PRECONDITION(condition) \
	if (!(condition)) { \
		PIO_PRECONDITION_FAILURE(#condition); \
		return -EINVAL; \
	}

/// Test @c pointer. If @c condition is NULL, log an error and return -EINVAL.
#define PIO_PRECONDITION_NOT_NULL(pointer) \
	if (!(pointer)) { \
		PIO_PRECONDITION_FAILURE(#pointer " is NULL"); \
		return -EINVAL; \
	}

#endif // !defined pio_log_h__included
