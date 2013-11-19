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

var types  = ['Disabled', 'GPIO1', 'GPIO2', 'GPIO3', 'PWM'];

function gpioSetMethod(gpioController, pin, value) {
	return {
		method: 'gpioWritePin',
		params: {
			gpioController: gpioController,
			pin: pin,
			value: value
		}
	};
}
function pwmWritePinMethod(pin, value) {
	return {
		method: 'pwmWritePin',
		params: {
			pwm: pin,
			dutyCycle: value
		}
	};
}
var methods = {
		'GPIO1' : function(pin, value) {return gpioSetMethod(1, pin, value);},
		'GPIO2' : function(pin, value) {return gpioSetMethod(2, pin, value);},
		'GPIO3' : function(pin, value) {return gpioSetMethod(3, pin, value);},
		'PWM'   : pwmWritePinMethod,
};

var gpioModes = ['unspecified', 'High impedance input', 'GPIO out', 'Special 1', 'Special 2'];
var gpioModeMap = {
	'High impedance input': 0,
	'GPIO out'  : 1,
	'Special 1' : 2,
	'Special 2' : 3
};

function Control($row, id, sequencer) {
	var $remove = $('<button>X</button>');
	var $add    = $('<button>+</button>');
	var timing = 1000;
	var $type  = $('<select>');
	for (var i = 0; i < types.length; i++)
	{
		var type = types[i];
		$type.append('<Option value="' + type + '">' + type + '</Option>');
	}
	var $pin    = $('<input type="number" value="0"/>');
	var $mode   = $('<select>');
	for (var i = 0; i < gpioModes.length; i++)
	{
		var mode = gpioModes[i];
		$mode.append('<Option value="' + mode + '">' + mode + '</Option>');
	}
	var $value  = $('<input type="number" value="0"/>');
	var $fire   = $('<button>Set now</button>');
	var $timing = $('<input type="number" value="' + timing + '">');
	addCell($row, $remove, {align: "center"});
	addCell($row, $add, {align: "center"});
	addCell($row, $type);
	addCell($row, $pin);
	addCell($row, $mode);
	addCell($row, $value);
	addCell($row, $fire);
	addCell($row, $timing);
	var $status = addCell($row);
	
	function getPin() {
		return parseInt($pin.val());
	}
	function getValue() {
		return parseInt($value.val());
	}
	function getTiming() {
		return parseInt($timing.val());
	}
	function clear() {
		$status.text("");
	}
	function handleType() {
		var val = $type.val();
		var isGpio = val.startsWith('GPIO');
		$mode.attr('disabled', !isGpio);
	}
	function handleMode() {
		var type = $type.val();
		if (!type.startsWith('GPIO'))
			return;
		var gpio = parseInt(type[4]);
		var pin = getPin();
		var mode = gpioModeMap[$mode.val()];
		if (mode == null)
			return;

		print('Setting gpio ' + gpio + ' pin ' + pin + ' to mode ' + mode);
		postCommand ({
					method: 'gpioSetMode',
					params: {
						gpioController: gpio,
						pin: pin,
						mode: mode
					}
				},
				function (result) {
					$status.text(value);
				},
				function(jqXHR, textStatus, errorThrown) {
					$status.text(textStatus + ': ' + toJson (errorThrown));
				}
		);
	}
	$mode.change(handleMode);
	$type.change(handleType);
	handleType();

	this.value  = getValue;
	this.timing = getTiming;
	this.clear  = clear;

	function fire() {
		clear();
		var type = $type.val();
		var pin = getPin();
		var value = getValue();
		console.log(type, pin, value);
		var method = methods[type];
		if (!method)
			return;
		console.log(method);
		method = method (pin, value);
		console.log(method);
		postCommand (
				method,
				function (result) {
					$status.text(value);
				},
				function(jqXHR, textStatus, errorThrown) {
					$status.text(textStatus + ': ' + toJson (errorThrown));
				}
		);
	};
	this.fire = fire;
	$fire.click(fire);

	$add.click(function() {
		console.log(sequencer);
		sequencer.addRow($row);
	});

	function remove() {
		sequencer.remove(id);
		$row.remove();
	}
	$remove.click(remove); // TODO: remove from controls
}

function Sequencer($parent) {
	var controls = [];
	var nextId = 0;
	var $sequencer = this;

	function _addRow($previousRow) {
		var $newRow = $('<tr>');
		$newRow.insertAfter($previousRow);
		var id = nextId++;
		controls.push(new Control($newRow, id, $sequencer));
	};
	this.addRow = _addRow;
	this.remove = function(id) {
		delete controls[id];
	};

	var $table = $('<table>');
	$parent.append($table);
	$table.append($('<caption>Sequencer<caption>'));
	var $tbody = $('<tbody>');
	$table.append($tbody);
	var $row = addRow($tbody);
	addHeaderCell($row); // delete
	var $add    = $('<button>+</button>');
	addHeaderCell($row, $add);
	$add.click(function() {
		_addRow($row);
	});
	addHeaderCell($row).text('Type');
	addHeaderCell($row).text('Pin');
	addHeaderCell($row).text('Mode');
	addHeaderCell($row).text('Value');
	addHeaderCell($row).text('Trigger');
	addHeaderCell($row).text('Timing / ms');
	addHeaderCell($row).text('');

	var $lastRow = addRow($tbody);
	var $run = $('<button>Fire timed sequence</button>');
	addCell($lastRow, null, {colspan: "7"});
	addCell($lastRow, $run);
	addCell($lastRow);

	$run.click(function(event) {
		// TODO: if we want more accurate timing, it should be done at the backend,
		// but the advantage of doing it here is that we can update the page directly
		// instead of requiring some more complex notification system (web sockets?)
		for (var i = 0; i < controls.length; i++) {
			var control = controls[i];
			if (control) {
				control.clear();
				window.setTimeout(control.fire, control.timing());
			}
		}
	});
}

function addSequencer() {
	$div = $('<div>');
	new Sequencer($div);
	$div.insertAfter($('#addSequencer'));
}


function init() {
	initPage();

	$('#addSequencer').click(addSequencer);
}

jQuery(document).ready(init);
