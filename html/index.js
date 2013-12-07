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

function init() {
	initPage();

	var version = $('#pixiVer');

	postCommand (
			{ method: 'getLibVersion' },
			function (result) {
				print ('Received getLibVersion');
				version.text (result);
			},
			function(jqXHR, textStatus, errorThrown) {
				version.text (textStatus + ': ' + errorThrown);
				print ('getLibVersion: ' + textStatus + ': ' + toJson (errorThrown));
			}
	);

	var piBoardRevision = $('#piBoardRevision');

	postCommand (
			{ method: 'getPiBoardRevision' },
			function (result) {
				print ('Received getPiBoardRevision');
				piBoardRevision.text (result);
			},
			function(jqXHR, textStatus, errorThrown) {
				piBoardRevision.text (textStatus + ': ' + errorThrown);
				print ('getPiBoardRevision: ' + textStatus + ': ' + toJson (errorThrown));
			}
	);

	var piBoardVersion = $('#piBoardVersion');

	postCommand (
			{ method: 'getPiBoardVersion' },
			function (result) {
				print ('Received getPiBoardVersion');
				piBoardVersion.text (result);
			},
			function(jqXHR, textStatus, errorThrown) {
				piBoardVersion.text (textStatus + ': ' + errorThrown);
				print ('getPiBoardVersion: ' + textStatus + ': ' + toJson (errorThrown));
			}
	);

	var pixiFpgaBuildTime = $('#pixiFpgaBuildTime');

	postCommand (
			{ method: 'pixiFpgaGetBuildTime' },
			function (result) {
				print ('Received pixiFpgaGetBuildTime');
				pixiFpgaBuildTime.text (new Date(1000 * result).toString());
			},
			function(jqXHR, textStatus, errorThrown) {
				pixiFpgaBuildTime.text (textStatus + ': ' + errorThrown);
				print ('pixiFpgaGetBuildTime: ' + textStatus + ': ' + toJson (errorThrown));
			}
	);

	postCommand (
			{ method: 'getCommands' },
			function (result) {
				print ('Received getCommands');
				var body = $('#commandTable').find('tbody');
				for (var i = 0; i < result.length; i++) {
					var cmd = result[i];
					var $row = addRow(body);
					addCell($row).text(cmd.method);
					addCell($row).text(cmd.description);
				}
			},
			function(jqXHR, textStatus, errorThrown) {
				print ('getCommands: ' + textStatus + ': ' + toJson (errorThrown));
			}
	);
}

jQuery(document).ready(init);
