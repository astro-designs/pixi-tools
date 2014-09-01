#    pixi-tools: a set of software to interface with the Raspberry Pi
#    and PiXi-200 hardware
#    Copyright (C) 2014 Simon Cantrill
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

from __future__ import print_function

from json import loads as decodeJson, dumps as encodeJson
from os import getenv
from threading import Thread
from time import sleep
from unittest import TestCase, main
import logging
import sys
if sys.version_info[0] > 2:
	from http.client import HTTPConnection
else:
	from httplib import HTTPConnection

log = logging.getLogger(__name__)
info = log.info

port = 9980

class TestLib (TestCase):

	def test_core (self):
		from pixitools import pi
		info ("Loaded: %s", pi)
		from pixitools import _pi
		info ("Loaded: %s", _pi)

		from pixitools import pixi
		info ("Loaded: %s", pixi)
		from pixitools import _pixi
		info ("Loaded: %s", _pixi)

		from pixitools.pi import getLibVersion
		ver = getLibVersion()
		assert isinstance (ver, str)
		info ("libpixi version: %s", ver)


def runCommand (method, **params):
	data = {}
	data["method"] = method
	if params:
		data["params"] = params
	params = encodeJson (data)
	headers = {'Content-Type': 'application/json'}
	con = HTTPConnection ('localhost', port)
	con.request ("POST", "/cmd", params, headers)
	response = con.getresponse()
	info ("Response status=%s, reason=%s", response.status, response.reason)
	data = response.read().decode ('UTF-8')
	return decodeJson (data)

class TestServer (TestCase):
	@staticmethod
	def setUpClass():
		from pixitools.httpd import Server
		global httpd
		info ("Setting up Server on port %s", port)
		httpd = Server (port)
		thr = Thread (target = httpd.serve_forever, name = "PiXi Server")
		thr.start()

	@staticmethod
	def tearDownClass():
		httpd.shutdown()

	def assertCommandError (self, response):
		# TODO: more detail
		assert isinstance (response, dict)
		self.assertEquals ('exception', response['.response'])
		assert response['message']

	def test_core (self):
		from pixitools.pi import getLibVersion, getPiBoardRevision, getPiBoardVersion
		libVer = getLibVersion()
		piRev  = getPiBoardRevision()
		piVer  = getPiBoardVersion()

		self.assertEqual (libVer, runCommand ('getLibVersion'))
		self.assertEqual (piRev , runCommand ('getPiBoardRevision'))
		self.assertEqual (piVer , runCommand ('getPiBoardVersion'))
		commands = runCommand ('getCommands')
		cmdMap = {}
		for cmd in commands:
			cmdMap[cmd['method']] = cmd
		assert 'getLibVersion' in cmdMap
		assert 'getPiBoardRevision' in cmdMap
		assert 'getPiBoardVersion' in cmdMap
		assert 'getCommands' in cmdMap

	def test_gpioSys (self):
		from pixitools.pi import gpioDirectionToStr, gpioSysGetPinState, GpioState
		assertEqual = self.assertEqual
		states = runCommand ('gpioSysGetStates')
		for state in states:
			localState = GpioState()
			pin = state['gpio']
			assert isinstance (pin, int)
			assert gpioSysGetPinState (pin, localState) >= 0
			assertEqual (gpioDirectionToStr (localState.direction), state['direction'])
			value = state['value']
			assert isinstance (value, int)
			assert value in (0, 1)
			assertEqual (value, localState.value)

	def test_gpio (self):
		from pixitools.pi import gpioDirectionToStr, gpioChipGetPinState, gpioMapWiringPiToChip, gpioMapRegisters, gpioUnmapRegisters, GpioState
		assertEqual = self.assertEqual
		states = runCommand ('gpioGetStates')
		assert gpioMapRegisters() >= 0
		try:
			for state in states:
				localState = GpioState()
				pin = state['gpio']
				assert isinstance (pin, int)
				assert gpioChipGetPinState (gpioMapWiringPiToChip (pin), localState) >= 0
				direction = state['direction']
				localDirection = gpioDirectionToStr (localState.direction)
				if not 'invalid' in localDirection:
					assertEqual (localDirection, direction)
				value = state['value']
				assert isinstance (value, int)
				assert value in (0, 1)
				assertEqual (value, localState.value)
		finally:
			gpioUnmapRegisters()

	def test_readWriteData (self):
		from shutil import rmtree
		from os.path import exists, join
		from os import getcwd
		import pixitools.commands
		datadir = join (getcwd(), 'pixitools-testdatadir')
		pixitools.commands.datadir = datadir
		if exists (datadir):
			rmtree (datadir)

		save = "some data"
		assert not runCommand ('writeData', group = 'testgroup', name = 'test1', data = save)
		assert not runCommand ('writeData', group = 'testgroup', name = 'test2', data = save + save)
		assert exists (join (datadir, 'testgroup', 'test1'))
		assert exists (join (datadir, 'testgroup', 'test2'))
		self.assertEqual (['test1', 'test2'], runCommand ('listDataGroup', group = 'testgroup'))
		self.assertEqual (save, runCommand ('readData', group = 'testgroup', name = 'test1'))
		self.assertEqual (save + save, runCommand ('readData', group = 'testgroup', name = 'test2'))
		assert exists (join (datadir, 'testgroup', 'test2'))

		self.assertCommandError (runCommand ('writeData', group = 'test.group', name = 'test2', data = save))
		self.assertCommandError (runCommand ('writeData', group = 'test/group', name = 'test2', data = save))
		self.assertCommandError (runCommand ('writeData', group = 'testgroup', name = 'te/st2', data = save))
		self.assertCommandError (runCommand ('writeData', group = 'testgroup', name = 'te.st2', data = save))
		# TODO: test for escape characters
		rmtree (datadir)

if __name__ == '__main__':
	logging.basicConfig (level = logging.INFO)
	if "no" == getenv ("TEST_HARDWARE"):
		del TestServer.test_gpioSys
		del TestServer.test_gpio
	main()
