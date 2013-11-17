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

var log;
var toJson = JSON.stringify
var fromJson = JSON.parse

if (!window.console)
	console = {
		log: function(){}
	};

function print(text) {
	log.innerHTML += text + '<br/>';
}

function dump(obj) {
	log.innerHTML += JSON.stringify (obj) + '<br/>';
}

function addCell(row, content) {
	var cell = $('<td>');
	if (content !== null) {
		cell.append(content);
	}
	row.append(cell);
	return cell;
}
function addRow(tbody) {
	row = $('<tr>');
	tbody.append(row);
	return row;
}

function postCommand (data, onSuccess, onError) {
	$.ajax({
		type: 'POST',
		url: '/cmd',
		processData: false,
		dataType: 'json',
		data: toJson (data),
		success: onSuccess,
		error: function(jqXHR, textStatus, errorThrown) {
			var responseText = jqXHR.responseText;
			if (responseText) {
				var response = fromJson(jqXHR.responseText);
				if (response != null) {
					console.log('command-exception', response);
					var type = response['.response'];
					if (type === 'exception') {
						if (onError != null)
							return onError(jqXHR, 'command-exception', response.message);
					}
				}
			}
			console.log('command-error', textStatus, errorThrown, jqXHR);
			if (onError != null)
				return onError(jqXHR, textStatus, errorThrown);
		}
	});
}

function initPage() {
	log = document.getElementById('log');
}
