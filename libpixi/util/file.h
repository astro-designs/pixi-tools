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

#ifndef libpixi_util_file_h__included
#define libpixi_util_file_h__included


#include <libpixi/common.h>
#include <stdlib.h>
#include <unistd.h>

LIBPIXI_BEGIN_DECLS

///@defgroup util_file libpixi file utilities
///@{

///	Pointer and size of a memory block
typedef struct Buffer
{
	void*   memory;
	size_t  size;
} Buffer;

///	Initialiser for Buffer
static const Buffer BufferInit = {NULL, 0};

///	Wraps up regular open().
///	Handles EINTR by retrying.
///	Adds O_CLOEXEC to flags.
///	Adds trace level debugging of status.
///	@return -errno on error, else a file descriptor
int pixi_open (const char* filename, int flags, int mode);

///	Wraps up regular close().
///	@return -errno on error, else 0
int pixi_close (int fd);

///	Wraps up regular read().
///	Handles EINTR by retrying with suitable buffer adjustment.
ssize_t pixi_read  (int fd,       void* buffer, size_t count);

///	Wraps up regular write().
///	Handles EINTR by retrying with suitable buffer adjustment.
ssize_t pixi_write (int fd, const void* buffer, size_t count);

///	Get the size of file @c fd.
///	@return file size, or -errno on error.
int64 pixi_fileGetSize (int fd);

/// Open a file, read contents into @c buffer, up to a limit of @c bufferSize.
/// On success, @c buffer will always be nul-terminated.
/// On error, -errno is returned.
ssize_t pixi_fileReadStr (const char* filename, char* buffer, size_t bufferSize);

/// Read a file, as with pixi_fileReadStr(), parse contents as an int and store in @c value.
/// On error -errno is returned.
int pixi_fileReadInt (const char* filename, int* value);

/// Open a file, and write @c str to it.
/// On error, -errno is returned.
ssize_t pixi_fileWriteStr (const char* filename, const char* value);

/// Open a file, and write @c str to it.
/// On error, -errno is returned.
ssize_t pixi_fileWriteInt (const char* filename, int value);

///	Opens @c filename, loads contents into a freshly allocated buffer
///	and stores that buffer in @c buffer. If successful, you must later call
///	free (buffer->buffer) to prevent a memory leak.
///	@return 0 on success, -errno on error.
int pixi_fileLoadContents (const char* filename, Buffer* buffer);

///	Loads contents of file @c fd into a freshly allocated buffer
///	and stores that buffer in @c buffer. If successful, you must later call
///	free (buffer->buffer) to prevent a memory leak.
///	The file is read from the current position.
///	@return 0 on success, -errno on error.
int pixi_fileLoadContentsFd (int fd, Buffer* buffer);

///@} defgroup

LIBPIXI_END_DECLS

#endif // !defined libpixi_util_file_h__included
