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
var toJson = JSON.stringify;
var fromJson = JSON.parse;
var updateObject = jQuery.extend;
var controlTypes = {};

if (typeof String.prototype.startsWith != 'function') {
	String.prototype.startsWith = function (str){
		return this.slice(0, str.length) == str;
	};
}

if (!window.console)
	console = {
		log: function(){}
	};

function escapeHtml (text) {
    var pre  = document.createElement ('pre');
    pre.appendChild (document.createTextNode (text));
    return pre.innerHTML;
}

function appendLog(html) {
	log.innerHTML = html + log.innerHTML;
}

function print(text) {
	appendLog(escapeHtml (text) + '<br/>');
}

function printError (text) {
	appendLog('<div class="errorLog">' + escapeHtml (text) + '</div>');
}

function dump(obj) {
	appendLog(JSON.stringify (obj) + '<br/>');
}

function clearLog() {
	log.innerHTML = '';
}

function dateString() {
	return new Date().toISOString();
}

function addTableItem(type, row, content, attributes) {
	var cell = $('<td>');
	if (content !== null) {
		cell.append(content);
	}
	if (attributes != null) {
		cell.attr(attributes);
	}
	row.append(cell);
	return cell;
}
function addCell(row, content, attributes) {
	return addTableItem('<td>', row, content, attributes);
}
function addHeaderCell(row, content, attributes) {
	return addTableItem('<th>', row, content, attributes);
}
function addRow(tbody) {
	row = $('<tr>');
	tbody.append(row);
	return row;
}
function addOption($parent, value) {
	value = escapeHtml(value);
	var $option = $('<option value="' + value + '">' + value + '</option>')
	$parent.append($option);
	return $option;
}
function setDatalist($datalist, values) {
	$datalist.html('');
	for (var i = 0; i < values.length; i++) {
		var value = escapeHtml(values[i]);
		$datalist.append('<option value="' + value + '"/>');
	}
}
function makeSelectFromList(list, initValue) {
	var $select = $('<select>');
	var index = null;
	for (var i = 0; i < list.length; i++)
	{
		var value = escapeHtml(list[i]);
		var $option = $('<option value="' + value + '">' + value + '</option>');
		$select.append($option);
		if (value == initValue)
			$option.attr('selected', true);
	}
	return $select;
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

function logPostCommand (data, onSuccess, onError) {
	postCommand(
			data,
			function (result) {
				print (dateString() + ': Received successful response from: ' + data.method);
				if (onSuccess)
					onSuccess(result);
			},
			function (jqXHR, textStatus, errorThrown) {
				error = data.method + ': ' + textStatus + ': ' + toJson (errorThrown);
				printError (dateString() + ': Received error response from ' + error);
				if (onError != null)
					onError (error);
			}
			);
}


function initPage() {
	log = document.getElementById('log');
	$('#clearLog').click(clearLog);
}
