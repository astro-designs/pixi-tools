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

#ifndef libpixi_pixi_uart_h__included
#define libpixi_pixi_uart_h__included


#include <libpixi/util/log.h>

LIBPIXI_BEGIN_DECLS

///@defgroup PiXiUART PiXi UART
///@{

/// Uart address map
typedef enum UartRegister
{
	TxFifo             = 0, ///< write (DLAB=0)
	RxFifo             = 0, ///< read  (DLAB=0)
	DivisorLatchLow    = 0, ///< read/write (DLAB=1)
	DivisorLatchHigh   = 1, ///< read/write (DLAB=1)
	InterruptEnableReg = 1, ///< read/write (DLAB=0)
	InterruptIdReg     = 2, ///< read
	FifoControlReg     = 2, ///< write
	LineControlReg     = 3, ///< read/write
	ModemControlReg    = 4, ///< read
	LineStatusReg      = 5, ///< read
	ModemStatusReg     = 6, ///< read
	ScratchReg         = 7, ///< read/write
} UartRegister;

/// FifoControlReg values
enum FifoControlBits
{
	EnableFifos = 1<<0,
	RxFifoReset = 1<<1,
	TxFifoReset = 1<<2,
	DmaMode1    = 1<<3,
	RxFifoTriggerLevel1Byte  = 0<<6,
	RxFifoTriggerLevel4Byte  = 1<<6,
	RxFifoTriggerLevel8Byte  = 2<<6,
	RxFifoTriggerLevel14Byte = 3<<6,
};

/// LineControlReg values
enum LineControlBits
{
	WordLength5  = 0<<0,
	WordLength6  = 1<<0,
	WordLength7  = 2<<0,
	WordLength8  = 3<<0,
	StopBits2    = 1<<2,
	ParityEnable = 1<<3,
	EvenParity   = 1<<4,
	StickParity  = 1<<5, ///< ignored
	SetBrake     = 1<<6,
	DivisorLatchAccess = 1<<7
};

/// LineStatusReg values
enum LineStatusBits
{
	DataReady         = 1<<0,
	OverrunError      = 1<<1,
	ParityError       = 1<<2,
	FramingError      = 1<<3,
	BreakInterrupt    = 1<<4,
	EmptyTxHoldingReg = 1<<5,
	EmptyTxReg        = 1<<6,
	ErrorInRxFifo     = 1<<7
};

enum
{
	IoBufferSize = 4096,
	IoBufferMask = IoBufferSize - 1
};

///	A lock-free queue buffer.
///	Note that the effective capacity is one less than IoBufferSize
///	because writePos==readPos means size==0, not size=IoBufferSize.
typedef struct IoBuffer
{
	byte  buffer[IoBufferSize];
	/*volatile*/ uint  writePos;
	/*volatile*/ uint  readPos;
} IoBuffer;

static inline void ioInit (IoBuffer* buf)
{
	buf->writePos = 0;
	buf->readPos = 0;
}

static inline bool ioIsEmpty (IoBuffer* buf)
{
	return (buf->readPos == buf->writePos);
}

///	Return size of data in buffer
static inline uint ioSize (IoBuffer* buf)
{
	uint readPos  = buf->readPos;
	uint writePos = buf->writePos;
	uint avail = writePos - readPos;
	if (writePos < readPos)
		avail += IoBufferSize;
	return avail;
}

///	Return size of data in buffer that's available in
///	in a single contiguous chunk
static inline uint ioContiguousSize (IoBuffer* buf)
{
	uint readPos  = buf->readPos;
	uint writePos = buf->writePos;
	if (writePos > readPos)
		return writePos - readPos;
	return IoBufferSize - readPos;
}

///	Push a single byte to the buffer. Return 0 if successful,
///	or a negative error value if out of space.
static inline int ioPush (IoBuffer* buf, byte value)
{
	uint readPos      = buf->readPos;
	uint writePos     = buf->writePos;
	uint nextWritePos = (writePos + 1) % IoBufferSize;
	LIBPIXI_PRECONDITION(nextWritePos < IoBufferSize);
//	LIBPIXI_LOG_TRACE("push readPos=0x%02x, writePos=0x%02x nextWritePos=0x%02x", readPos, writePos, nextWritePos);
	if (nextWritePos == readPos)
		return -ENOBUFS;

	buf->buffer[writePos] = value;
	buf->writePos = nextWritePos;
	return 0;
}

///	Pop a single byte from the buffer. Return the value if successful,
///	or a negative error code if no data is available.
static inline int ioPop (IoBuffer* buf)
{
	uint readPos = buf->readPos;
	if (readPos == buf->writePos)
		return -ENODATA;
	byte value = buf->buffer[readPos];
	readPos = (readPos + 1) & IoBufferMask;
	buf->readPos = readPos;
	return value;
}

static inline uint ioWrite (IoBuffer* buf, const void* data, uint size)
{
	// TODO: rewrite in optimal fashion
	uint i;
	const byte* bytes = (byte*) data;
	for (i = 0; i < size; i++)
	{
		int result = ioPush (buf, bytes[i]);
		if (result < 0)
			return i;
	}
	return size;
}

static inline uint ioRead  (IoBuffer* buf, void* data, uint size)
{
	// TODO: rewrite in optimal fashion
	uint i;
	byte* bytes = (byte*) data;
	for (i = 0; i < size; i++)
	{
		int value = ioPop (buf);
		if (value < 0)
			return i;
		bytes[i] = value;
	}
	return size;
}

typedef struct Uart
{
	uint8     address;
	uint      baudRate;
	uint      status;
	uint      prevStatus;
	uint      operations; ///< LineStatusBits that were use in operation
	uint      softErrors; ///< Bitmap of software errors, such as OverrunError;
	IoBuffer  txBuf;
	IoBuffer  rxBuf;
} Uart;

///	Open a UART at the given address, set the baud-rate and clear buffers.
int pixi_uartOpen (Uart* uart, uint address, uint baudRate);

///	Open a UART at the given address, set the baud-rate and clear buffers.
int pixi_uartDebugOpen (Uart* uart, uint address, uint baudRate);

///	Get the status of @c count uarts, processing data read/write as requird.
int pixi_uartProcess (Uart* uarts, uint count);

uint getBaudDivisor (uint baudRate);

int setBaudRate (Uart* uart);

///@} defgroup

LIBPIXI_END_DECLS


#endif // !defined libpixi_pixi_uart_h__included
