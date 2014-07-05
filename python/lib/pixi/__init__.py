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

import logging
from os import strerror
from functools import update_wrapper

def basicLogging (level = logging.INFO):
	logging.basicConfig (level = level)

class PixiToolsError (RuntimeError):
	def __init__(self, functionName, result):
		super (PixiToolsError, self).__init__(functionName + " failed: " + strerror (-result))
		self.result = result

def _makeWrapper (name, defWrapper, wrapped):
	def wrapper (*args):
		result = wrapped (*args)
		if result < 0:
			raise PixiToolsError (name, result)
		return result
	update_wrapper (wrapper, defWrapper)
	return wrapper

def _rewrap (wrapper, wrapped):
	"""Converts int returning functions to exception raising"""
	for name, obj in wrapper.items():
		# select pure python wrappers
		if callable (obj):
			try:
				wrappedFn = wrapped[name]
				if isinstance (wrappedFn.__doc__, str) and ' -> int' in wrappedFn.__doc__:
					wrapper[name] = _makeWrapper (name, obj, wrappedFn)
			except KeyError:
				pass
