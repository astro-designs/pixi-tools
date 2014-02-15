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

function SequencerRow($row, sequencer, remove, config) {
	var self = this;
	this.$row = $row;
	var conf = {
			'type': 'Disabled',
			'pin': 1,
			'mode': 'unspecified',
			'value': 0,
			'timing': 1000
	};
	updateObject(conf, config);

	// TODO: apply config
	this.remove = remove;
	var $remove = $('<button>X</button>');
	var $add    = $('<button>+</button>');
	var $type   = makeSelectFromList(types, conf.type);
	var $pin    = $('<input type="number" style="width:4em"/>');
	$pin.val(conf.pin);
	var $mode = makeSelectFromList(gpioModes, conf.mode);
	var $value  = $('<input type="number" style="width:4em"/>');
	$value.val(conf.value);
	var $fire   = $('<button>Set now</button>');
	var $timing = $('<input type="number" style="width:6em" value="' + conf.timing + '">');
	addCell($row, $remove, {align: "center"});
	addCell($row, $add, {align: "center"});
	addCell($row, $type);
	addCell($row, $pin);
	addCell($row, $mode);
	addCell($row, $value);
	addCell($row, $fire);
	addCell($row, $timing);
	var $status = addCell($row);

	function getType() {
		return $type.val();
	}
	function getPin() {
		return parseInt($pin.val());
	}
	function getMode() {
		return $mode.val();
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
		var val = getType();
		var isGpio = val.startsWith('GPIO');
		$mode.attr('disabled', !isGpio);
	}
	function handleMode() {
		var type = $type.val();
		if (!type.startsWith('GPIO'))
			return;
		var gpio = parseInt(type[4]);
		var pin = getPin();
		var modeInt = gpioModeMap[getMode()];
		if (mode == null)
			return;

		print('Setting gpio ' + gpio + ' pin ' + pin + ' to mode ' + mode);
		logPostCommand ({
					method: 'gpioSetMode',
					params: {
						gpioController: gpio,
						pin: pin,
						mode: mode
					}
				},
				function (result) {
					$status.text (result);
				},
				function (error) {
					$status.text (error);
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
		logPostCommand (
				method,
				function (result) {
					$status.text(value);
				},
				function (error) {
					$status.text (error);
				}
		);
	};
	this.fire = fire;
	$fire.click(fire);

	$add.click(function() {
		console.log(sequencer);
		sequencer.addRow(self);
	});

	$remove.click(remove);

	this.getConfig = function() {
		var config = {
				'.class': 'SequencerRow',
				'type': getType(),
				'pin': getPin(),
				'mode': getMode(),
				'value': getValue(),
				'timing': getTiming()
		};
		return config;
	};
}

function Sequencer(config) {
	var rows = [];
	var sequencer = this;

	var _addRow = function(prevSeqRow, config) {
		var $previousRow;
		if (prevSeqRow != null)
			$previousRow = prevSeqRow.$row;
		else
			$previousRow = $headerRow;
		var $newRow = $('<tr>');
		$newRow.insertAfter($previousRow);
		var seqRow = new SequencerRow(
				$newRow,
				sequencer,
				function() {
					$newRow.remove();
					rows.splice(rows.indexOf(seqRow), 1);
					console.log("remove row", rows);
				},
				config
		);
		if (prevSeqRow == null)
			rows.unshift(seqRow);
		else {
			var index = rows.indexOf(prevSeqRow);
			rows.splice(index+1, 0, seqRow);
		}
		console.log("added row", rows);
		return seqRow;
	};
	this.addRow = _addRow;

	var $table = $('<table>');
	this.$widget = $table;
	var $tbody = $('<tbody>');
	$table.append($tbody);
	var $headerRow = addRow($tbody);
	addHeaderCell($headerRow);
	var $add    = $('<button>+</button>');
	addHeaderCell($headerRow, $add);
	$add.click(function() {_addRow();});

	addHeaderCells($headerRow, ['Type', 'Pin', 'Mode', 'Value', 'Trigger', 'Timing / ms', '']);

	var $lastRow = addRow($tbody);
	var $run = $('<button style="width:100%">Fire timed sequence</button>');
	addCell($lastRow, null, {colspan: "6"});
	addCell($lastRow, $run, {colspan: "2"});
	addCell($lastRow);

	$run.click(function(event) {
		// TODO: if we want more accurate timing, it should be done at the backend,
		// but the advantage of doing it here is that we can update the page directly
		// instead of requiring some more complex notification system (web sockets?)
		for (var i = 0; i < rows.length; i++) {
			var row = rows[i];
			if (row) {
				row.clear();
				window.setTimeout(row.fire, row.timing());
			}
		}
	});

	if (config != null) {
		var rowConf = config['rows'];
		var current = null;
		for (id in rowConf) {
			var conf = rowConf[id];
			current = this.addRow(current, conf);
		}
	}

	this.getConfig = function() {
		var rowConf = [];
		var config = {'.class': 'Sequencer', 'rows': rowConf};
		for (var i = 0; i < rows.length; i++) {
			rowConf.push(rows[i].getConfig());
		}
		return config;
	};
}

controlTypes['Sequencer'] = Sequencer;
