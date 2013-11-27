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

from pixitools import pi
from pixitools.pi import gpioSysGetPinState, gpioGetPinState, gpioMapRegisters, gpioUnmapRegisters, gpioDirectionToStr, gpioEdgeToStr, GpioState, SpiDevice
from pixitools.pixi import pixiSpiOpen, registerWrite as _registerWrite, pixiGpioSetPinMode as _pixiGpioSetPinMode
from pixitools.pixi import pixiGpioWritePin as _pixiGpioWritePin, pwmWritePin as _pwmWritePin, pwmWritePinPercent as _pwmWritePinPercent
from pixitools.pixix import Lcd, Spi, check
import logging

log = logging.getLogger(__name__)
info = log.info
debug = log.debug

class CommandError (RuntimeError):
	def __init__ (self, message):
		super (CommandError, self).__init__ (message)

class InvalidCommand (CommandError):
	def __init__ (self, message):
		super (InvalidCommand, self).__init__ (message)

commands = {}
commandList = []

def addCommand (method, doc = None):
	if doc is None:
		doc = method.__doc__
	commands[method.__name__] = method
	commandList.append ({'method': method.__name__, 'description': doc})

for func, doc in (
	(pi.getLibVersion     , 'Get the libpixi version'),
	(pi.getPiBoardRevision, 'Get the Raspberry Pi board revision'),
	(pi.getPiBoardVersion , 'Get the Raspberry Pi board version'),
	):
	addCommand (func, doc)

def getCommands():
	'Get a list of commands'
	return commandList
addCommand (getCommands)

def gpioGetStates():
	'Get the states of all gpios'
	if gpioMapRegisters() < 0:
		raise CommandError ("Could not map GPIO registers")
	try:
		states = []
		for n in range (20):
			gs = GpioState()
			if gpioGetPinState (n, gs) >= 0:
				direction = gpioDirectionToStr (gs.direction)
				if "invalid" in direction:
					direction = "mode:0x%x" % gs.direction
				state = {
					'gpio'     : n,
					'direction': direction,
					'value'    : gs.value,
					}
				states.append (state)
		return states
	finally:
		gpioUnmapRegisters()
addCommand (gpioGetStates)

def gpioSysGetStates():
	'Get the states of all gpios from the /sys/ interface'
	states = []
	for n in range (64):
		gs = GpioState()
		if gpioSysGetPinState (n, gs) >= 0:
			state = {
				'gpio'     : n,
				'exported' : gs.exported,
				'direction': gpioDirectionToStr (gs.direction),
				'edge'     : gpioEdgeToStr (gs.edge),
				'value'    : gs.value,
				'activeLow': gs.activeLow
				}
			states.append (state)
	return states
addCommand (gpioSysGetStates)

def registerWrite (address, value):
	'Write to a PiXi register via SPI'
	global spi
	if not spi:
		spi = Spi()
	if _registerWrite (spi.spi, address, value) < 0:
		raise CommandError ("Failed to write SPI value")
addCommand (registerWrite)

lcd = None
def lcdSetText (text):
	'Set the LCD panel text'
	text = str (text)
	global lcd
	if not lcd:
		lcd = Lcd()
	lcd.setText (text)
addCommand (lcdSetText)

spi = None
def gpioWritePin (gpioController, pin, value):
	'Set the output value of a PiXi GPIO pin'
	global spi
	if not spi:
		spi = Spi()
	check (_pixiGpioWritePin (spi.spi, gpioController, pin, value))
addCommand (gpioWritePin)

spi = None
def gpioSetMode (gpioController, pin, mode):
	'Set the mode of a PiXi GPIO pin'
	global spi
	if not spi:
		spi = Spi()
	check (_pixiGpioSetPinMode (spi.spi, gpioController, pin, mode))
addCommand (gpioSetMode)

spi = None
def pwmWritePin (pwm, dutyCycle):
	'Set the state of the PiXi PWM'
	global spi
	if not spi:
		spi = Spi()
	check (_pwmWritePin (spi.spi, pwm, dutyCycle))
addCommand (pwmWritePin)

spi = None
def pwmWritePinPercent (pwm, dutyCycle):
	'Set the state of the PiXi PWM'
	global spi
	if not spi:
		spi = Spi()
	check (_pwmWritePinPercent (spi.spi, pwm, dutyCycle))
addCommand (pwmWritePinPercent)

def processCommand (command):
	methodName = command.get ('method')
	if methodName is None:
		raise InvalidCommand ('Missing method in request: ' + repr (command))
	method = commands.get (methodName)
	if method is None:
		raise InvalidCommand ('Unknown method in request: '  + methodName)

	params = command.get ('params')
	if params is None:
		return method()
	else:
		return method (**params)
