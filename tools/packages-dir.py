#    pixi-tools: a set of software to interface with the Raspberry Pi
#    and PiXi-200 hardware
#    Copyright (C) 2013 Simon Cantrill
#
#    pixi-tools is free software; you can redistribute it and/or
#    modify it under the terms of the GNU Lesser General Public
#    License as published by the Free Software Foundation; either
#    version 2.1 of the License, or (at your option) any later version.
#
#    This library is distributed in the hope that it will be useful,
#    but WITHOUT ANY WARRANTY; without even the implied warranty of
#    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
#    Lesser General Public License for more details.
#
#    You should have received a copy of the GNU Lesser General Public
#    License along with this library; if not, write to the Free Software
#    Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA

# Determine whether to use site-packages or dist-packages

import sys
from os.path import basename, dirname, join, normpath

if len (sys.argv) != 2:
	raise ValueError ("must provide basic python path")

packages = "site-packages"
inputPath = sys.argv[1]
base = normpath (inputPath)

for path in sys.path:
	parent = dirname (path)
	if parent == base:
		child = basename (path)
		if child in ('site-packages', 'dist-packages'):
			packages = child
			break

install = join (inputPath, packages)
print (install)
