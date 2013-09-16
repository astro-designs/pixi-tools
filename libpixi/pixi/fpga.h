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

#ifndef libpixi_pixi_fpga_h__included
#define libpixi_pixi_fpga_h__included


#include <libpixi/util/file.h>

LIBPIXI_BEGIN_DECLS

///@defgroup PixiFpga PiXi-200 FPGA interface
///@{

///	Load an FPGA image from a file to the PiXi.
///	@return 0 on success, -errno on error.
int pixi_pixiFpgaLoadFile (const char* filename);

///	Load an FPGA image from a buffer to the PiXi.
///	@return 0 on success, -errno on error.
int pixi_pixiFpgaLoadBuffer (const Buffer* buffer);

///	Get the version of the FPGA from the PiXi.
///	@return >=0 on success, -errno on error.
int64 pixi_pixiFpgaGetVersion (void);

///@} defgroup

LIBPIXI_END_DECLS

#endif // !defined libpixi_pixi_fpga_h__included
