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

#ifndef libpixi_pixi_registers_h__included
#define libpixi_pixi_registers_h__included

#include <libpixi/common.h>

LIBPIXI_BEGIN_DECLS

///@defgroup PixiRegisters PiXi-200 register maps
///@{

///	hardware registers
typedef enum PixiRegisters
{
	// Configuration & Status TODO
	Pixi_FPGA_build_time0                 = 0x00, ///< 16 bit read
	Pixi_FPGA_build_time1                 = 0x01, ///< 16 bit read
	Pixi_FPGA_build_time2                 = 0x02, ///< 16 bit read

	// SPI & I2C configuration
	Pixi_I2C_config                       = 0x08, ///< Read/Write 16 bit
	_Pixi_I2C_config_FPGA_SPI_channel_bit       = 1<<0, ///< 1 bit register: sets PiXi FPGA SPI channel to 0 or 1 [default=0]
	_Pixi_I2C_config_MCP3204_SPI_channel_bit    = 1<<1, ///< 1 bit register: sets the MCP3204 4-ch ADC SPI channel [default=1]

	Pixi_SPI_config                       = 0x09, ///< Read/Write 16 bit
	// TODO: 7 bit or 8 bit?
	_Pixi_SPI_config_address_bit                = 1<<0, ///< 7 bit register: sets PiXi FPGA I2C address [default=0x70]
	_Pixi_SPI_config_inhibit_STOP_bit           = 1<<8, ///< 1 bit register: inhibits an I2C STOP from propagating to any slave.

	// Raspberry Pi I/O
	Pixi_PI_GPIO_cfg0                     = 0x10, ///< Write only 16 bit

	Pixi_PI_GPIO_cfg1                     = 0x11, ///< Write only 16 bit

	// GPIO
	Pixi_GPIO1_00_07_IO                   = 0x20, ///< 8 bit. Read/Write for GPIO1(0-7).
	Pixi_GPIO1_08_15_IO                   = 0x21, ///< 8 bit. Read/Write for GPIO1(8-15).
	Pixi_GPIO1_16_23_IO                   = 0x22, ///< 8 bit. Read/Write for GPIO1(15-23).

	// TODO: in/out or readback/out?
	Pixi_GPIO2_00_07_IO                   = 0x23, ///< 8 bit. Read/Write for GPIO2(0-7).
	Pixi_GPIO2_08_15_IO                   = 0x24, ///< 8 bit. Read/Write for GPIO2(8-15).

	Pixi_GPIO3_00_07_IO                   = 0x25, ///< 8 bit. Read/Write for GPIO2(0-7).
	Pixi_GPIO3_08_15_IO                   = 0x26, ///< 8 bit. Read/Write for GPIO2(8-15).

	Pixi_GPIO1_00_07_mode                 = 0x27, ///< 16 bit. Read/Write: Arranged as pairs of bits, (1:0), (3:2), .. (14:15)
	Pixi_GPIO1_08_15_mode                 = 0x28, ///< Read/Write 16 bit
	Pixi_GPIO1_16_23_mode                 = 0x29, ///< Read/Write 16 bit
	_Pixi_GPIO_modes_input                      = 0x0, ///< GPIO pin is driven as a high impedance (input mode)
	_Pixi_GPIO_modes_output                     = 0x1, ///< GPIO pin is driven from GPIO out register
	_Pixi_GPIO_modes_special_1                  = 0x2, ///< GPIO pin is driven from a special function (TBD)
	_Pixi_GPIO_modes_special_2                  = 0x3, ///< GPIO pin is driven from a special function (TBD)

	Pixi_GPIO2_00_07_mode                 = 0x2A, ///< Read/Write 16 bit
	_Pixi_GPIO2a_modes_PWM                       = _Pixi_GPIO_modes_special_1, ///< GPIO pin is driven from PWM controller (0..7)
	Pixi_GPIO2_08_15_mode                 = 0x2B, ///< Read/Write 16 bit
	_Pixi_GPIO2b_modes_GND                       = _Pixi_GPIO_modes_special_1, ///< GPIO pin is driven to GND

	Pixi_GPIO3_00_07_mode                 = 0x2C, ///< Read/Write 16 bit
	Pixi_GPIO3_08_15_mode                 = 0x2D, ///< Read/Write 16 bit
	_Pixi_GPIO3_modes_LCD_VFD                   = _Pixi_GPIO_modes_special_1, ///< GPIO pin is driven from the LCD / VFD interface

	Pixi_LEDs_out                         = 0x30,
	Pixi_LEDs_config                      = 0x31,
	Pixi_Switch_in                        = 0x32, ///< 8 bit read. Describes status of the four push button switches
	Pixi_Keypad                           = 0x33,
	Pixi_VFDLCD_out                       = 0x38,

	Pixi_PWM0_control                     = 0x40,
	Pixi_PWM1_control                     = 0x41,
	Pixi_PWM2_control                     = 0x42,
	Pixi_PWM3_control                     = 0x43,
	Pixi_PWM4_control                     = 0x44,
	Pixi_PWM5_control                     = 0x45,
	Pixi_PWM6_control                     = 0x46,
	Pixi_PWM7_control                     = 0x47,

	Pixi_PWM0_config                      = 0x48,
	Pixi_PWM1_config                      = 0x49,
	Pixi_PWM2_config                      = 0x4A,
	Pixi_PWM3_config                      = 0x4B,
	Pixi_PWM4_config                      = 0x4C,
	Pixi_PWM5_config                      = 0x4D,
	Pixi_PWM6_config                      = 0x4E,
	Pixi_PWM7_config                      = 0x4F,
} PixiRegisters;

///@} defgroup

LIBPIXI_END_DECLS

#endif // !defined libpixi_pixi_registers_h__included
