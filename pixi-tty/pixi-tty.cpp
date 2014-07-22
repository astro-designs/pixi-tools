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

#include <libpixi/libpixi.h>
#include <libpixi/util/log.h>
#include <libpixi/util/string.h>
#include <libpixi/pixi/uart.h>
#include <libpixi/pixi/simple.h>
#include <locale.h>
#include <unistd.h>
#include <algorithm>
#include <cstdio>
#include <cstring>
#include <thread>
#include <mutex>
#include <condition_variable>
#include <cuse_lowlevel.h>


using std::thread;
using std::mutex;
using std::condition_variable;
typedef std::unique_lock<mutex> mutex_lock;


namespace { // anon


cuse_lowlevel_ops uartOps;


class UartDev
{
	UartDev (const UartDev& other) = delete;
	void operator = (const UartDev& other) = delete;

public:
	UartDev() :
		m_devNum (-1),
		m_uart   (0)
	{
		m_name[0] = 0;
	}
	~UartDev()
	{
		if (m_thread.joinable())
			m_thread.join();
	}

	const char* name() const {
		return m_name;
	}

	int start (uint devNum, Uart* uart)
	{
		m_devNum = devNum;
		m_uart   = uart;
		snprintf (m_name, sizeof (m_name), "ttyPIXI%u", devNum);

		const char* args[] = {"pixi-uart", "-f", 0}; // "-f": foreground (don't fork)
		char nameInfo[20];
		snprintf (nameInfo, sizeof (nameInfo), "DEVNAME=%s", name());
		const char* devInfo[] = {nameInfo, "MODE=0666"};

		cuse_info cinfo;
		memset (&cinfo, 0, sizeof (cinfo));
		cinfo.dev_info_argc = 1;
		cinfo.dev_info_argv = devInfo;
		cinfo.flags = CUSE_UNRESTRICTED_IOCTL;

		int multiThreaded = 1;
		struct fuse_session* session = cuse_lowlevel_setup (
			2, const_cast<char**> (args),
			&cinfo,
			&uartOps,
			&multiThreaded,
			this
			);
		if (!session)
		{
			int err = errno;
			APP_ERRNO_FATAL("cuse_lowlevel_setup failed");
			return -err;
		}

		m_thread = thread ([session]() {
			int result = fuse_session_loop_mt (session);
			if (result < 0) {
				APP_ERRNO_ERROR("fuse_session_loop_mt failed");
			}
		});
		return 0;
	}
	uint read (char* data, uint size, int flags)
	{
		APP_LOG_DEBUG("read of size %u, available=%u", size, ioSize (&m_uart->rxBuf));

		mutex_lock lock (m_mutex);
		if (flags & O_NONBLOCK)
		{
			uint count = ioRead (&m_uart->rxBuf, data, size);
			APP_LOG_DEBUG("non-block read, count=%u", count);
			return count;
		}

		while (true)
		{
			uint count = ioRead (&m_uart->rxBuf, data, size);
			APP_LOG_DEBUG("partial read, count=%u", count);
			if (count > 0)
				return count;
			m_condition.wait (lock);
		}
		return 0; // unreachable
	}
	uint write (const char* data, uint size, int flags)
	{
		APP_LOG_DEBUG("write of size %u, available=%u", size, ioSize (&m_uart->rxBuf));

		mutex_lock lock (m_mutex);
		if (flags & O_NONBLOCK)
		{
			uint count = ioWrite (&m_uart->txBuf, data, size);
			APP_LOG_DEBUG("non-block write, count=%u", count);
			return count;
		}

		uint remain = size;
		while (true)
		{
			uint count = ioWrite (&m_uart->txBuf, data, remain);
			APP_LOG_DEBUG("partial write, count=%u", count);
			if (count == remain)
				return size;
			data   += count;
			remain -= count;
			m_condition.wait (lock);
		}
		return size; // unreachable
	}

	void process() {
		if ((m_uart->operations & DataReady)
			|| (m_uart->operations & EmptyTxHoldingReg))
		{
			mutex_lock lock (m_mutex);
			APP_LOG_DEBUG("notify_all because 0x%02x", m_uart->operations);
			m_condition.notify_all();
		}
	}

private:
	mutex               m_mutex;
	condition_variable  m_condition;

	uint        m_devNum;
	char        m_name[12];
	Uart*       m_uart;
	thread      m_thread;
};


} // namespace (anon)

inline UartDev& uartDev (const fuse_file_info* fileInfo)
{
	return *reinterpret_cast<UartDev*> (fileInfo->fh);
}

static void uart_open (fuse_req_t req, struct fuse_file_info* fi)
{
	UartDev* dev = reinterpret_cast<UartDev*> (fuse_req_userdata (req));
	APP_LOG_INFO("open: dev=%s flags=%x ", dev->name(), fi->flags);
	fi->fh = (ulong) dev;
	fi->nonseekable = true;
	fuse_reply_open (req, fi);
}

static const char printableChars[] = " 0123456789ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz,./<>?;:'@#~][}{=-+_`!\"$^&*()";

static void uart_read (fuse_req_t req, size_t size, off_t off, struct fuse_file_info* fi)
{
	LIBPIXI_UNUSED(off);
	fi->nonseekable = true;

	UartDev& dev = uartDev (fi);

	char buffer[4096];
	uint count = dev.read (buffer, std::min (sizeof (buffer), size), fi->flags);
	APP_LOG_INFO("read data = %u", count);

	char printable[1+(sizeof(buffer)*3)];
	pixi_hexEncode (buffer, count, printable, sizeof (printable), '%', printableChars);
	APP_LOG_DEBUG("read: [%s]", printable);
	if (count == 0)
		fuse_reply_err (req, EAGAIN);
	else
		fuse_reply_buf (req, buffer, count);
}

static void uart_write (fuse_req_t req, const char *buf, size_t size, off_t off, struct fuse_file_info* fi)
{
	fi->nonseekable = true;

	UartDev& dev = uartDev (fi);
	APP_LOG_DEBUG("write %s size=%zu off=%llu", dev.name(), size, (ulonglong) off);

	uint count = dev.write (buf, size, fi->flags);
	APP_LOG_INFO("wrote data = %u", count);
	if (count == 0)
		fuse_reply_err (req, EAGAIN);
	else
		fuse_reply_write (req, count);
}

static void uart_ioctl (
	fuse_req_t req,
	int cmd,
	void *arg,
	struct fuse_file_info *fi,
	unsigned int flags,
      const void *in_buf,
      size_t in_bufsz,
      size_t out_bufsz)
{
	UartDev& dev = uartDev (fi);
	APP_LOG_INFO("ioctl %s cmd=%d arg=%p flags=%xu in_bufsz=%zu out_bufsz=%zu", dev.name(), cmd, arg, flags, in_bufsz, out_bufsz);
	fi->nonseekable = true;
	fuse_reply_ioctl(req, 0, NULL, 0);
}

int main (int argc, char* argv[])
{
	setlocale (LC_ALL, "");

	pixi_appLogLevel = pixi_strToLogLevel (getenv ("PIXI_TTY_LOG_LEVEL"), LogLevelInfo);

	uint rate = 38400;
	if (argc > 1)
		rate = pixi_parseLong (argv[1]);

	uartOps.open     = uart_open;
	uartOps.read     = uart_read;
	uartOps.write    = uart_write;
	uartOps.ioctl    = uart_ioctl;
/*	uartOps.release  =;
	uartOps.poll     =;
*/
	pixiOpenOrDie();


	Uart uarts[4];
	int result = pixi_uartDebugOpen (&uarts[0], 0x80, rate);
	if (result < 0)
	{
		APP_LOG_FATAL("Could not open PiXi uart");
		return 255;
	}

	UartDev devs[4];
	uint index = 0;
	for (UartDev& dev: devs)
	{
		int result = dev.start (index, &uarts[0]);
		if (result < 0)
			return result;
		index++;
	}

	while (true)
	{
		int result = pixi_uartProcess (uarts, 1);
		if (result < 0)
			return result;
		devs[0].process();
	}
}
