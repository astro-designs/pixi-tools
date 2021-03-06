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

	logPostCommand (
			{ method: 'gpioGetStates' },
			function (result) {
				fillStateTable (result);
			}
	);
}

function fillStateTable (states) {
	var body = $('#gpioStatesTable').find('tbody');
	for (var i = 0; i < states.length; i++) {
		var state = states[i];
		var row = addRow(body);
		addCell(row).text(state.gpio);
		addCell(row).text(state.direction);
		addCell(row).text(state.value);
	}
}

jQuery(document).ready(init);
