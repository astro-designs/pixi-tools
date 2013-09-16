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

#define _LARGEFILE64_SOURCE
#include <libpixi/util/file.h>
#include <libpixi/util/log.h>
#include <errno.h>
#include <fcntl.h>
#include <stdio.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>

static const size_t ContentsMaxSize = 0x40000000; ///< Arbitrary limit

int pixi_open (const char* filename, int flags, int mode)
{
	LIBPIXI_PRECONDITION_NOT_NULL(filename);

	int result = open (filename, flags | O_CLOEXEC, mode);
	if (result < 0)
	{
		result = -errno;
		LIBPIXI_ERRNO_TRACE("error in pixi_open (filename=\"%s\", flags=%d, mode=%d)", filename, flags, mode);
		return result;
	}
	LIBPIXI_LOG_TRACE("%d = pixi_open (filename=\"%s\", flags=%d, mode=%d)", result, filename, flags, mode);
	return result;
}

int pixi_close (int fd)
{
	// Whether to retry on EINTR seems to be disputed
	// In http://lkml.indiana.edu/hypermail/linux/kernel/0509.1/0877.html, Linus wrote:
	//    If close() return EINTR, the file descriptor _will_ have been closed. The
	//    error return just tells you that soem error happened on the file: for
	//    example, in the case of EINTR, the close() may not have flushed all the
	//    pending data synchronously.
	int result = close (fd);
	if (result < 0)
	{
		result = -errno;
		LIBPIXI_ERRNO_WARN("error in pixi_close (fd=%d)", fd);
	}
	return result;
}

ssize_t pixi_read (int fd, void* buffer, size_t count)
{
	LIBPIXI_PRECONDITION_NOT_NULL(buffer);
	LIBPIXI_PRECONDITION(count > 0);

	ssize_t bytesIn = 0;
	ssize_t result = 0;
	size_t bytesToRead = count;
	byte* _buffer = (byte*) buffer;
	do
	{
		errno = 0;
		result = read (fd, _buffer, bytesToRead);
		if (result > 0)
		{
			bytesIn     += result;
			_buffer     += result;
			bytesToRead -= result;
		}
	}
	while (bytesIn < (ssize_t) count && errno == EINTR);
	if (result < 0) {
		result = -errno;
		LIBPIXI_ERRNO_WARN("error in pixi_read (fd=%d, buffer=%p, count=%zu)", fd, buffer, count);
	}
	return bytesIn > 0 ? bytesIn : result;
}

ssize_t pixi_write (int fd, const void* buffer, size_t count)
{
	LIBPIXI_PRECONDITION_NOT_NULL(buffer);
	LIBPIXI_PRECONDITION(count > 0);

	ssize_t bytesOut = 0;
	ssize_t result = 0;
	size_t bytesToWrite = count;
	const byte* _buffer = (const byte*) buffer;
	do
	{
		errno = 0;
		result = write (fd, _buffer, bytesToWrite);
		if (result > 0)
		{
			bytesOut     += result;
			_buffer      += result;
			bytesToWrite -= result;
		}
	}
	while (bytesOut < (ssize_t) count && errno == EINTR);
	if (result < 0) {
		result = -errno;
		LIBPIXI_ERRNO_WARN("error in pixi_write (fd=%d, buffer=%p, count=%zu)", fd, buffer, count);
	}
	return bytesOut > 0 ? bytesOut : result;
}

int64 pixi_fileGetSize (int fd)
{
	LIBPIXI_PRECONDITION(fd >= 0);

	//	fd is usually a regular file, so try stat() first.
	struct stat64 st;
	int result = fstat64 (fd, &st);
	if (result < 0)
	{
		int err = errno;
		LIBPIXI_ERROR(err, "fstat of file failed");
		return -err;
	}
	if (S_ISREG(st.st_mode) || st.st_size > 0)
		return st.st_size;

	//	This is primarily for block devices, where stat.st_size is 0,
	//	even though they are seekable.
	//	save current position:
	int64 cur = lseek64 (fd, 0, SEEK_CUR);
	if (cur < 0)
	{
		int err = errno;
		LIBPIXI_ERROR(err, "lseek of file failed");
		return -err;
	}
	int64 end = lseek64 (fd, 0, SEEK_END);
	if (end < 0)
	{
		int err = errno;
		LIBPIXI_ERROR(err, "second lseek of file failed");
		return -err;
	}
	lseek (fd, cur, SEEK_SET);
	return end;

}

ssize_t pixi_fileReadStr (const char* filename, char* buffer, size_t bufferSize)
{
	LIBPIXI_PRECONDITION_NOT_NULL(buffer);
	LIBPIXI_PRECONDITION(bufferSize > 0);

	int fd = pixi_open (filename, O_RDONLY, 0);
	if (fd < 0)
		return fd;
	ssize_t count = pixi_read (fd, buffer, bufferSize - 1);
	if (count >= 0)
		buffer[count] = '\0';
	pixi_close (fd);
	return count;
}

int pixi_fileReadInt (const char* filename, int* value)
{
	LIBPIXI_PRECONDITION_NOT_NULL(value);

	char buf[40];
	int result = pixi_fileReadStr (filename, buf, sizeof (buf));
	if (result < 0)
		return result;
	*value = atoi (buf);
	return 0;
}

ssize_t pixi_fileWriteStr (const char* filename, const char* value)
{
	LIBPIXI_PRECONDITION_NOT_NULL(filename);
	LIBPIXI_PRECONDITION(value != NULL);

	size_t len = strlen (value);

	int fd = pixi_open (filename, O_WRONLY, 0);
	if (fd < 0)
		return fd;
	ssize_t result = pixi_write (fd, value, len);
	pixi_close (fd);
	return result;
}

ssize_t pixi_fileWriteInt (const char* filename, int value)
{
	char buf[40];
	snprintf (buf, sizeof (buf), "%d", value);
	return pixi_fileWriteStr (filename, buf);
}


int pixi_fileLoadContentsFd (int fd, Buffer* buffer)
{
	LIBPIXI_PRECONDITION(fd >= 0);
	LIBPIXI_PRECONDITION_NOT_NULL(buffer);

	*buffer = BufferInit;

	int64 fileSize = pixi_fileGetSize (fd);
	if (fileSize <= 0)
		return fileSize;
	else if (fileSize > (int64) ContentsMaxSize)
	{
		LIBPIXI_LOG_ERROR(
			"Refusing to load file contents: size of %lld exceeds limit of %zu bytes",
			(long long) fileSize, ContentsMaxSize);
		return -ERANGE;
	}
	else
	{
		size_t size = fileSize;
		void* memory = malloc (size);
		if (!memory)
		{
			int err = errno;
			free (memory);
			LIBPIXI_LOG_ERROR("Memory allocation error: failed to allocate %zu bytes", size);
			return -err;
		}
		ssize_t count = pixi_read (fd, memory, size);
		if (count < 0)
		{
			int err = errno;
			free (memory);
			LIBPIXI_ERROR(err, "file read error");
			return -err;
		}
		// count could be less than size if file did not start at 0,
		// or if it shrank before reading. Either way, we've read what we can.
		if (count < (ssize_t) size)
		{
			LIBPIXI_LOG_TRACE("Short read on file. Wanted %zu bytes, received %zd", size, count);
			memory = realloc (memory, size);
		}
		buffer->memory = memory;
		buffer->size   = size;
		return 0;
	}
	return -EILSEQ; // unreachable
}

int pixi_fileLoadContents (const char* filename, Buffer* buffer)
{
	LIBPIXI_PRECONDITION_NOT_NULL(filename);
	LIBPIXI_PRECONDITION_NOT_NULL(buffer);

	int fd = pixi_open (filename, O_RDONLY, 0);
	if (fd < 0)
		return fd;

	int result = pixi_fileLoadContentsFd (fd, buffer);
	pixi_close (fd);
	return result;
}
