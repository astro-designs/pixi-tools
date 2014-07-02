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
#include <libpixi/pixi/uart.h>
#include <libpixi/util/string.h>
#include "Command.h"
#include "log.h"
#include <stdio.h>
#include <sys/time.h>

static inline int setUartReg (Uart* uart, UartRegister reg, uint8 value)
{
	return registerWrite (uart->address + reg, value);
}
static inline int getUartReg (Uart* uart, UartRegister reg)
{
	return registerRead (uart->address + reg);
}

static uint getStatusReg (Uart* uart)
{
	uint status = getUartReg (uart, LineStatusReg);
	if (status != uart->prevStatus)
	{
//		PIO_LOG_INFO("Status-reg: 0x%02x", status);
		uart->prevStatus = status;
		PIO_LOG_TRACE("Uart 0x%2x status register=0x%02x", uart->address, status);
//		if (status & DataReady        ) PIO_LOG_TRACE("   DataReady: at least one character in receive FIFO");
//		if (status & EmptyTxHoldingReg) PIO_LOG_TRACE("   EmptyTxHoldingReg: transmitter FIFO is empty");
//		if (status & EmptyTxReg       ) PIO_LOG_TRACE("   EmptyTxReg: transmitter FIFO and shift register is empty");
		if (status & OverrunError     ) PIO_LOG_ERROR("   OverrunError: receive FIFO was full, received character lost");
		if (status & ParityError      ) PIO_LOG_ERROR("   ParityError: top character in FIFO was received with a parity error");
		if (status & FramingError     ) PIO_LOG_ERROR("   FramingError: top character in FIFO was received without a valid stop bit");
		if (status & BreakInterrupt   ) PIO_LOG_WARN ("   BreakInterrupt: a break condition has been reached");
		if (status & ErrorInRxFifo    ) PIO_LOG_DEBUG("   ErrorInRxFifo: at least one error in receiver FIFO");
	}

	return status;
}

static void handleUart (Uart* uart)
{
	uint status;
	do
	{
		status = getStatusReg (uart);
		if ((status & DataReady) && ioSize (&uart->rxBuf) < (IoBufferSize - 1))
		{
			byte value = getUartReg (uart, RxFifo);
			ioPush (&uart->rxBuf, value);
		}
		if (status & EmptyTxHoldingReg)
		{
			int value = ioPop (&uart->txBuf);
			if (value >= 0)
				setUartReg (uart, TxFifo, value);
		}
	} while (((status & DataReady) && ioSize (&uart->rxBuf) < (IoBufferSize - 1))
		|| ((status & EmptyTxHoldingReg) && !ioIsEmpty (&uart->txBuf)));
}

static ssize_t uartWrite (Uart* uart, const void* buffer, size_t size)
{
	PIO_LOG_DEBUG("Writing %zu bytes", size);
	uint written = ioWrite (&uart->txBuf, buffer, size);
	if (written < size)
		PIO_LOG_ERROR("Overflow of %zu bytes in internal uart txBuf", size - written);
	return written;
}

static ssize_t uartRead (Uart* uart, void* buffer, size_t size)
{
	return ioRead (&uart->rxBuf, buffer, size);
}

static const char printableChars[] = " 0123456789ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz,./<>?;:'@#~][}{=-+_`!\"$^&*()";


static int uartReadWriteMonitorFn (const Command* command, uint argc, char* argv[])
{
	if (argc != 3)
		return commandUsageError (command);

	Uart uart;
	uint address  = pixi_parseLong (argv[1]);
	uint baudRate = pixi_parseLong (argv[2]);

	pixiOpenOrDie();
	pixi_uartOpen (&uart, address, baudRate);

	const char* msg = "Starting read-write-loop\r\n";
	uartWrite (&uart, msg, strlen (msg));

	while (true)
	{
		handleUart (&uart);
		char buf[100];
		ssize_t count = uartRead (&uart, buf, sizeof (buf));
		if (count > 0)
		{
			char printable[1+(sizeof(buf)*3)];
			if (pio_isLogLevelEnabled (LogLevelDebug))
			{
				pixi_hexEncode (buf, count, printable, sizeof (printable), ' ', "");
				PIO_LOG_DEBUG("Received [as hex]: [%s]", printable);
			}
			pixi_hexEncode (buf, count, printable, sizeof (printable), '%', printableChars);
			char out[sizeof (printable) + 40];
			PIO_LOG_DEBUG("Received: [%s]", printable);
			size_t len = snprintf (out, sizeof (out), "Received: [%s]\r\n", printable);
			uartWrite (&uart, out, len + 2);
		}
		else
			usleep (40);
	}
	pixiClose();
	return 0;
}
static Command uartReadWriteMonitorCmd =
{
	.name        = "uart-rw-monitor",
	.description = "read writing from a UART, writing back a description of input",
	.usage       = "usage: %s UART BAUDRATE",
	.function    = uartReadWriteMonitorFn
};

static uint testScratch (Uart* uart)
{
	setUartReg (uart, ScratchReg, 0x55);
	bool ok = (0x55 == getUartReg (uart, ScratchReg));
	setUartReg (uart, ScratchReg, 0xaa);
	ok |= (0xaa == getUartReg (uart, ScratchReg));
	if (ok)
	{
		PIO_LOG_INFO("Scratch register verified");
		return 0;
	}
	else
	{
		PIO_LOG_ERROR("Scratch register write/read failure");
		return 8;
	}
}

static void testWrite (Uart* uart, uint count)
{
	PIO_LOG_INFO("Testing writes for approximately 2 seconds");

	const uint writes = 2 * (uart->baudRate / 10);
	uint statusReads = 0;

	struct timeval start;
	gettimeofday (&start, NULL);
	byte value = 0;
	for (uint i = 0; i < writes; i++)
	{
		for (uint u = 0; u < count; u++)
			ioPush (&uart[u].txBuf, value++);
		uint operation;
		do
		{
			statusReads++;
			pixi_uartProcess (uart, count);
			operation = uart->operations; // TODO: check each UART
		} while (!(EmptyTxHoldingReg & operation));
	}
	struct timeval end;
	gettimeofday (&end, NULL);
	uint startUs = start.tv_usec + (start.tv_sec * 1000000);
	uint endUs   = end  .tv_usec + (end  .tv_sec * 1000000);
	double sec   = (endUs - startUs) / 1000000.0;
	PIO_LOG_INFO("Finished 2 seconds of worth of writes in %.6f seconds", sec);
	PIO_LOG_INFO("Required %u status register reads for %u output bytes", statusReads, writes);
}

static int uartTestFn (const Command* command, uint argc, char* argv[])
{
	if (argc < 3 || argc > 6)
		return commandUsageError (command);

	pixiOpenOrDie();

	uint count = argc - 2;
	Uart uarts[4];
	memset (uarts, 0, sizeof (uarts));
	uint baudRate = pixi_parseLong (argv[1]);
	uint address  = pixi_parseLong (argv[2]);
	pixi_uartDebugOpen (&uarts[0], address, baudRate);
	uint result = testScratch (&uarts[0]);
	for (uint i = 1; i < count; i++)
	{
		address  = pixi_parseLong (argv[i+2]);
		pixi_uartDebugOpen (&uarts[i], address, baudRate);
		result |= testScratch (&uarts[i]);
	}

	testWrite (uarts, count);
	pixiClose();
	return result ? -EIO : 0;
}
static Command uartTestCmd =
{
	.name        = "uart-test",
	.description = "do a brief test of a UART channel",
	.usage       = "usage: %s BAUDRATE UART1 [UART2] [UART3] [UART4]",
	.function    = uartTestFn
};

static const Command* commands[] =
{
	&uartReadWriteMonitorCmd,
	&uartTestCmd,
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
