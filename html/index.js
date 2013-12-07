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

	logPostCommand (
			{ method: 'getLibVersion' },
			function (result) {
				version.text (result);
			},
			function (error) {
				version.text (error);
			}
	);

	var piBoardRevision = $('#piBoardRevision');

	logPostCommand (
			{ method: 'getPiBoardRevision' },
			function (result) {
				piBoardRevision.text (result);
			},
			function (error) {
				piBoardRevision.text (error);
			}
	);

	var piBoardVersion = $('#piBoardVersion');

	logPostCommand (
			{ method: 'getPiBoardVersion' },
			function (result) {
				piBoardVersion.text (result);
			},
			function (error) {
				piBoardVersion.text (error);
			}
	);

	var pixiFpgaBuildTime = $('#pixiFpgaBuildTime');

	logPostCommand (
			{ method: 'pixiFpgaGetBuildTime' },
			function (result) {
				pixiFpgaBuildTime.text (new Date(1000 * result).toString());
			},
			function (error) {
				pixiFpgaBuildTime.text (error);
			}
	);

	logPostCommand (
			{ method: 'getCommands' },
			function (result) {
				var body = $('#commandTable').find('tbody');
				for (var i = 0; i < result.length; i++) {
					var cmd = result[i];
					var $row = addRow(body);
					addCell($row).text(cmd.method);
					addCell($row).text(cmd.description);
				}
			}
	);
}

jQuery(document).ready(init);
