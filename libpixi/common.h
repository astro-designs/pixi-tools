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

#ifndef libpixi_common_h__included
#define libpixi_common_h__included


#include <stdbool.h>
#include <stdint.h>

typedef          long long   longlong;
typedef unsigned long long  ulonglong;

typedef unsigned char  byte;
typedef   signed char  schar;
typedef unsigned char  uchar;
typedef unsigned short ushort;
typedef unsigned int   uint;
typedef unsigned long  ulong;

typedef  int8_t    int8;
typedef uint8_t   uint8;
typedef  int16_t   int16;
typedef uint16_t  uint16;
typedef  int32_t   int32;
typedef uint32_t  uint32;
typedef  int64_t   int64;
typedef uint64_t  uint64;

typedef  intptr_t  intptr;
typedef uintptr_t uintptr;

#define LIBPIXI_UNUSED(var) (void) (var)
#define ARRAY_COUNT(array) (sizeof (array) / sizeof (array[0]))

#if defined __GNUC__
#	define LIBPIXI_PRINTF_ARG(arg) __attribute__((format (printf, arg, arg+1)))
#	define LIBPIXI_CONSTRUCTOR(priority) __attribute__((constructor (10000 + priority)))
#	define LIBPIXI_DEPRECATED __attribute__((deprecated))
///	Says a function's return value must be used.
///	g++ < 4 supports this attribute, but produces spurious warnings.
#	define LIBPIXI_USE_RESULT __attribute__((warn_unused_result))
///	An alternate name for LIBSC_USE_RESULT when the result is an error value.
#	define LIBPIXI_MUST_CHECK __attribute__((warn_unused_result))
#else
#	define LIBPIXI_PRINTF_ARG(arg)
#	define LIBPIXI_CONSTRUCTOR(priority)
#	define LIBPIXI_DEPRECATED
#	define LIBPIXI_USE_RESULT
#	define LIBPIXI_MUST_CHECK
#endif


#if defined __cplusplus && __cplusplus >= 201103L
#	define LIBPIXI_STATIC_ASSERT static_assert
#elif __STDC_VERSION__ >= 201112L
#	define LIBPIXI_STATIC_ASSERT _Static_assert
#else
	/// Compile-time assertion, compatible with static_assert in C11
	/// If @c test evaluates as false, you will get a compilation error.
	// Implementation note: if test is false, the _static_assert array will have a negative size.
#	define LIBPIXI_STATIC_ASSERT(test,message) {char _static_assert[1-(2*!(test))]; (void)_static_assert;}
#endif

#if defined __cplusplus
#	define LIBPIXI_BEGIN_DECLS extern "C" {
#	define LIBPIXI_END_DECLS }
#else
#	define LIBPIXI_BEGIN_DECLS
#	define LIBPIXI_END_DECLS
#endif

///	Branch prediction hints for gcc compiler
#define LIBPIXI_LIKELY(  expression) __builtin_expect(!!(expression), 1)
#define LIBPIXI_UNLIKELY(expression) __builtin_expect(!!(expression), 0)

#endif // !defined libpixi_common_h__included
