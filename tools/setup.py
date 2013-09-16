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

#!/usr/bin/env python

from glob import glob
from distutils.core import setup

scripts = glob ('python/scripts/*')
scripts = list (filter (lambda name: not name.endswith('~'), scripts))

options = {
	'build': {
		'build_base': 'build/python-setup'
		}
	}

setup (
	name        = 'pixi-tools',
	description = 'Python interface to Pi/Pixi-200',
	author      = 'Simon Cantrill',
	scripts     = scripts,
	packages    = ['pixitools'],
	package_dir = {'pixitools': 'python/lib/pixi'},
	options     = options
	)
