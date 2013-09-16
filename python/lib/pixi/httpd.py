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

from __future__ import print_function

import sys
from os import chdir
from json import loads as decodeJson, dumps as encodeJson
from pixitools.commands import InvalidCommand, processCommand
if sys.version_info[0] > 2:
	from http.server import HTTPServer
	from http.server import SimpleHTTPRequestHandler
else:
	from BaseHTTPServer import HTTPServer
	from SimpleHTTPServer import SimpleHTTPRequestHandler
import logging

log = logging.getLogger(__name__)
info = log.info
debug = log.debug

# TODO: see about using a framework
# for starters, see http://docs.python.org/2/library/wsgiref.html

commandPath = '/cmd'

class RequestHandler (SimpleHTTPRequestHandler):
	def __init__ (self, request, client_address, server):
		info ('client_address: %s', client_address)
		SimpleHTTPRequestHandler.__init__(self, request, client_address, server)

	def do_POST (self):
		if self.path.startswith (commandPath):
			self.handleCommand()
		else:
			self.sendMethodNotAllowed()

	def do_HEAD (self):
		if self.path.startswith (commandPath):
			self.sendMethodNotAllowed()
		else:
			return SimpleHTTPRequestHandler.do_HEAD(self)

	def do_GET (self):
		if self.path.startswith (commandPath):
			self.sendMethodNotAllowed()
		else:
			return SimpleHTTPRequestHandler.do_GET(self)

	def handleCommand (self):
		self.dumpRequest()
		try:
			length = self.headers['content-length']
			if length is None:
				raise InvalidCommand ('Content-length is absent')
			length = int (length)
			data = self.rfile.read (length)
			debug ('data: %s', data)
			command = decodeJson (data.decode ('UTF-8'))
			debug ('decoded-json: %s', command)
			info ('Incoming command: [%s]', command.get ('method'))
			result = processCommand (command)
			self.sendResponse (('HTTP/1.0 200 OK',
					    'Content-Type: application/json'
					    ),
					   encodeJson (result)
					   )

		except InvalidCommand as e:
			log.exception ("exception handling command")
			# TODO: better errors
			self.sendMethodNotAllowed (e)

		except Exception as e:
			log.exception ("exception handling command")
			jsonEx = {
				".response": 'exception',
				'message': str(e)
				}
			self.sendInternalError (jsonEx)

	def sendResponse (self, headers, data = ''):
		response = '\r\n'.join (headers) + '\r\n\r\n' + data
		debug ('Sending response: %s', repr (response))
		self.wfile.write (response.encode ('UTF-8'))

	def sendMethodNotAllowed (self):
		self.sendResponse (('HTTP/1.0 405 Method Not Allowed',))

	def sendInternalError (self, json):
		self.sendResponse (('HTTP/1.0 500 Internal Server Error',
				    'Content-Type: application/json'
				    ),
				   encodeJson (json))

	def dumpRequest (self):
		for attr in 'client_address', 'command', 'path', 'request_version', 'headers':
			debug ('%s: %s', attr, getattr (self, attr))

class Server (HTTPServer):
	def __init__ (self, port, host = ''):
		HTTPServer.__init__(self, (host, port), RequestHandler)
