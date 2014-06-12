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

#include <libpixi/pixi/simple.h>
#include <libpixi/util/string.h>
#include "Command.h"
#include "log.h"
#include <stdio.h>


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
	ModemStatusReg     = 4, ///< read
	LineStatusReg      = 5  ///< read
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

typedef struct Uart
{
	uint8   basePort;
	uint    baudRate;
	uint    lastStatus;
	// buffers, etc.
} Uart;

static inline int setUartReg (Uart* uart, UartRegister reg, uint8 value)
{
	return registerWrite (uart->basePort + reg, value);
}
static inline int getUartReg (Uart* uart, UartRegister reg)
{
	return registerRead (uart->basePort + reg);
}

static inline uint getControlReg (Uart* uart)
{
	uint value = getUartReg (uart, LineControlReg);
	PIO_LOG_DEBUG("control register=0x%02x", value);
	return value;
}
static inline void setControlReg (Uart* uart, uint8 value)
{
	PIO_LOG_DEBUG("control register setting to 0x%02x", value);
	setUartReg (uart, LineControlReg, value);
}

static void setBaudRate (Uart* uart)
{
	uint divisor = (25 * 1000 * 1000) / (16 * uart->baudRate);
	PIO_LOG_INFO("Setting uart baud rate of %u [%u]", uart->baudRate, divisor);
	uint control = getControlReg (uart);
	setControlReg (uart, control | DivisorLatchAccess);
	setUartReg (uart, DivisorLatchLow , (uint8)  divisor);
	setUartReg (uart, DivisorLatchHigh, (uint8) (divisor >> 8));
	setControlReg (uart, control & ~DivisorLatchAccess);
}

static uint getStatusReg (Uart* uart)
{
	uint status = getUartReg (uart, LineStatusReg);
	if (status != uart->lastStatus)
	{
//		PIO_LOG_INFO("Status-reg: 0x%02x", status);
		uart->lastStatus = status;
		PIO_LOG_TRACE("Uart 0x%2x status register=0x%02x", uart->basePort, status);
//		if (status & DataReady        ) PIO_LOG_TRACE("   DataReady: at least one character in receive FIFO");
//		if (status & EmptyTxHoldingReg) PIO_LOG_TRACE("   EmptyTxHoldingReg: transmitter FIFO is empty");
//		if (status & EmptyTxReg       ) PIO_LOG_TRACE("   EmptyTxReg: transmitter FIFO and shift register is empty");
		if (status & OverrunError     ) PIO_LOG_DEBUG("   OverrunError: receive FIFO was full, received character lost");
		if (status & ParityError      ) PIO_LOG_DEBUG("   ParityError: top character in FIFO a was received with a parity error");
		if (status & FramingError     ) PIO_LOG_DEBUG("   FramingError: top character in FIFO a was received without a valid stop bit");
		if (status & BreakInterrupt   ) PIO_LOG_DEBUG("   BreakInterrupt: a break condition has been reached");
		if (status & ErrorInRxFifo    ) PIO_LOG_DEBUG("   ErrorInRxFifo: at least one error in receiver FIFO");
	}

	return status;

}

static ssize_t uartWrite (Uart* uart, const char* buffer, size_t size)
{
	PIO_LOG_INFO("Writing %zu bytes", size);
	const char* end = buffer + size;
	while (buffer < end)
	{
		while (!(getStatusReg (uart) & EmptyTxHoldingReg))
			usleep(100);
		setUartReg (uart, TxFifo, *buffer);
		buffer++;
	}
	PIO_LOG_INFO("Wrote %zu bytes", size);
	return size;
}

static ssize_t uartRead (Uart* uart, char* buffer, size_t size)
{
	char* buf = buffer;
	const char* limit = buffer + size;
	while (buf < limit && (getStatusReg (uart) & DataReady)) {
		*buf = getUartReg (uart, RxFifo);
		buf++;
	}
	return buf - buffer;
}

static const char printableChars[] = " 0123456789ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz,./<>?;:'@#~][}{=-+_`!\"$^&*()";


static void initUart (Uart* uart)
{
	setBaudRate (uart);
	uart->lastStatus = -1;

	setUartReg (uart, FifoControlReg, RxFifoTriggerLevel1Byte | EnableFifos | RxFifoReset | TxFifoReset);
	setUartReg (uart, LineControlReg, WordLength8);
	setUartReg (uart, InterruptEnableReg, 0); // disable interrupts
}

static int uartReadWriteMonitorFn (const Command* command, uint argc, char* argv[])
{
	if (argc != 3)
		return commandUsageError (command);

	Uart uart;
	uart.basePort = pixi_parseLong (argv[1]);
	uart.baudRate = pixi_parseLong (argv[2]);

	pixiOpenOrDie();
	initUart (&uart);

	const char* msg = "Starting read-write-loop\r\n";
	uartWrite (&uart, msg, strlen (msg));

	while (true)
	{
		char buf[100];
		ssize_t count = uartRead (&uart, buf, sizeof (buf));
		if (count > 0 && pio_isLogLevelEnabled(LogLevelDebug))
		{
			char printable[1+(sizeof(buf)*3)];
			pixi_hexEncode (buf, count, printable, sizeof (printable), ' ', "");
			PIO_LOG_DEBUG("Received [as hex]: [%s]", printable);
			pixi_hexEncode (buf, count, printable, sizeof (printable), '%', printableChars);
			char out[sizeof (printable) + 40];
			size_t len = snprintf (out, sizeof (out), "Received: [%s]", printable);
			PIO_LOG_INFO(out, printable);
			strcat (out, "\r\n");
			uartWrite (&uart, out, len + 2);
		}
		else
			usleep (1000);
	}
	return 0;

	pixiClose();
	return 0;
}
static Command uartReadCmd =
{
	.name        = "uart-read-write-monitor",
	.description = "read from a UART channel",
	.usage       = "usage: %s UART BAUDRATE",
	.function    = uartReadWriteMonitorFn
};

static const Command* commands[] =
{
	&uartReadCmd,
};


static CommandGroup pixiUartGroup =
{
	.name      = "pixi-uart",
	.count     = ARRAY_COUNT(commands),
	.commands  = commands,
	.nextGroup = NULL
};

static void PIO_CONSTRUCTOR (109) initGroup (void)
{
	addCommandGroup (&pixiUartGroup);
}
