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

#include <libpixi/pixi/uart.h>
#include <libpixi/pixi/simple.h>

enum
{
	UartClock = 17663043
};

static inline int setUartReg (Uart* uart, UartRegister reg, uint8 value)
{
	return registerWrite (uart->address + reg, value);
}
static inline int getUartReg (Uart* uart, UartRegister reg)
{
	return registerRead (uart->address + reg);
}

static inline uint getControlReg (Uart* uart)
{
	int value = getUartReg (uart, LineControlReg);
	LIBPIXI_LOG_DEBUG("control register=0x%02x", value);
	return value;
}
static inline int setControlReg (Uart* uart, uint8 value)
{
	LIBPIXI_LOG_DEBUG("Setting UART 0x%02x control register setting to 0x%02x", uart->address, value);
	return setUartReg (uart, LineControlReg, value);
}

uint getBaudDivisor (uint baudRate)
{
	if (baudRate == 0)
	{
		LIBPIXI_LOG_ERROR("Cannot set baudRate of 0");
		return 0;
	}
	return UartClock / (16 * baudRate);
}

int setBaudRate (Uart* uart)
{
	uint divisor = getBaudDivisor (uart->baudRate);
	LIBPIXI_LOG_INFO("Setting uart baud rate of %u [%u]", uart->baudRate, divisor);
	int control = getControlReg (uart);
	if (control < 0)
		return control;

	setControlReg (uart, control | DivisorLatchAccess);
	setUartReg (uart, DivisorLatchLow , (uint8)  divisor);
	setUartReg (uart, DivisorLatchHigh, (uint8) (divisor >> 8));
	return setControlReg (uart, control & ~DivisorLatchAccess);
}

int pixi_uartOpen (Uart* uart, uint address, uint baudRate)
{
	LIBPIXI_PRECONDITION_NOT_NULL(uart);
	uart->address    = address;
	uart->baudRate   = baudRate;
	uart->status     = -1;
	uart->prevStatus = -1;
	uart->operations = 0;
	uart->softErrors = 0;
	ioInit (&uart->txBuf);
	ioInit (&uart->rxBuf);
	int result = setBaudRate (uart);
	if (result < 0)
		LIBPIXI_ERROR(-result, "Failed to set baud rate of UART at address 0x%02x", address);

	setUartReg (uart, FifoControlReg, RxFifoTriggerLevel1Byte | EnableFifos | RxFifoReset | TxFifoReset);
	setUartReg (uart, LineControlReg, WordLength8);
	setUartReg (uart, InterruptEnableReg, 0); // disable interrupts

	return result;
}

int pixi_uartDebugOpen (Uart* uart, uint address, uint baudRate)
{
	LIBPIXI_PRECONDITION_NOT_NULL(uart);
	uart->address    = address;
	uart->baudRate   = baudRate;
	uart->status     = -1;
	uart->prevStatus = -1;
	uart->operations = 0;
	uart->softErrors = 0;
	ioInit (&uart->txBuf);
	ioInit (&uart->rxBuf);

	uint result = 0;
	LIBPIXI_LOG_INFO("Testing UART at address 0x%02x", uart->address);
	uint control = getControlReg (uart);
	LIBPIXI_LOG_INFO("Control register = 0x%02x, setting divisor latch access", control);
	setControlReg (uart, control | DivisorLatchAccess);
	uint divisor = getBaudDivisor (uart->baudRate);
	LIBPIXI_LOG_INFO("Setting baud rate to %u, divisor to 0x%02x", uart->baudRate, divisor);
	uint8 lo = divisor;
	uint8 hi = divisor >> 8;
	setUartReg (uart, DivisorLatchLow , lo);
	setUartReg (uart, DivisorLatchHigh, hi);
	uint8 lo2 = getUartReg (uart, DivisorLatchLow);
	uint8 hi2 = getUartReg (uart, DivisorLatchHigh);
	if (lo == lo2 && hi == hi2)
		LIBPIXI_LOG_INFO("Baud rate verified");
	else
	{
		result |= 4;
		LIBPIXI_LOG_ERROR("Baud rate read back is wrong [0x%02x, 0x%02x]", lo2, hi2);
	}
	setControlReg (uart, control & ~DivisorLatchAccess);

	lo2 = getUartReg (uart, DivisorLatchLow);
	hi2 = getUartReg (uart, DivisorLatchHigh);
	if (lo == lo2 && hi == hi2)
	{
		result |= 2;
		LIBPIXI_LOG_WARN("With divisor latch access disabled, same values from baud-rate registers");
	}

	setUartReg (uart, FifoControlReg, RxFifoTriggerLevel1Byte | EnableFifos | RxFifoReset | TxFifoReset);
	setUartReg (uart, LineControlReg, WordLength8);
	setUartReg (uart, InterruptEnableReg, 0); // disable interrupts
	return result;
}

static void checkErrors (const Uart* uart)
{
	uint status = uart->status;
	if (status != uart->prevStatus)
	{
		if (status & OverrunError     ) LIBPIXI_LOG_ERROR("UART 0x%02x: OverrunError: receive FIFO was full, received character lost", uart->address);
		if (status & ParityError      ) LIBPIXI_LOG_ERROR("UART 0x%02x: ParityError: top character in FIFO was received with a parity error", uart->address);
		if (status & FramingError     ) LIBPIXI_LOG_ERROR("UART 0x%02x: FramingError: top character in FIFO was received without a valid stop bit", uart->address);
		if (status & BreakInterrupt   ) LIBPIXI_LOG_WARN ("UART 0x%02x: BreakInterrupt: a break condition has been reached", uart->address);
		if (status & ErrorInRxFifo    ) LIBPIXI_LOG_DEBUG("UART 0x%02x: ErrorInRxFifo: at least one error in receiver FIFO", uart->address);
	}
}

int pixi_uartProcess (Uart* uarts, uint count)
{
	// Read all of the status registers
	RegisterOp ops[count];
	for (uint i = 0; i < count; i++)
	{
		ops[i].address = uarts[i].address + LineStatusReg;
		ops[i].function = PixiSpiEnableRead16;
	}
	int result = pixi_multiRegisterOp (ops, count);
	if (result < 0)
	{
		LIBPIXI_ERROR(-result, "Failed to read UART(s) status");
		return result;
	}
	int operations = 0;
	// Check the status bits and prepare for data read/write
	RegisterOp data[count*2];
	uint op = 0;
	for (uint i = 0; i < count; i++)
	{
		Uart* ut = &uarts[i];
		uint status = ops[i].value;
		ut->prevStatus = ut->status;
		ut->status     = status;
		ut->operations = 0;
		checkErrors (ut);
		if ((status & DataReady) && ioSize (&ut->rxBuf) < (IoBufferSize - 1))
		{
//			LIBPIXI_LOG_DEBUG("Preparing to read a byte");
			data[op].address  = ut->address + RxFifo;
			data[op].function = PixiSpiEnableRead16;
			op++;
			ut->operations |= DataReady;
		}
		if (status & EmptyTxHoldingReg)
		{
			int value = ioPop (&ut->txBuf);
			if (value >= 0)
			{
//				LIBPIXI_LOG_DEBUG("Writing byte 0x%02x", value);
				data[op].address  = ut->address + TxFifo;
				data[op].function = PixiSpiEnableWrite16;
				data[op].value    = value;
				op++;
				ut->operations |= EmptyTxHoldingReg;
			}
		}
		LIBPIXI_LOG_TRACE("Uart 0x%02x status=0x%02x operations=0x%02x", ut->address, status, ut->operations);
		operations |= ut->operations;
	}
	if (op == 0)
		return 0; // nothing to do
	result = pixi_multiRegisterOp (data, count);
	if (result < 0)
	{
		LIBPIXI_ERROR(-result, "Failed to read/write UART FIFO(s)");
		return result;
	}
	// Handle data reads
	op = 0;
	for (uint i = 0; i < count; i++)
	{
		Uart* ut = &uarts[i];
		ut->softErrors = 0;
		if (ut->operations & DataReady)
		{
			result = ioPush (&ut->rxBuf, data[op].value);
			if (result < 0)
			{
				LIBPIXI_LOG_ERROR("Overflow in software buffer for UART 0x02%x", ut->address);
				ut->softErrors |= OverrunError;
			}
			op++;
		}
		if (ut->operations & EmptyTxHoldingReg)
			op++;
	}
	return operations;
}
