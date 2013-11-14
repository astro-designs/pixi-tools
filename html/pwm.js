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

function PwmUnit(index, $row) {
	var timing = 1000 * (index + 1);
	var $value  = $('<input type="number" value="100">'); // TODO: limits
	var $fire   = $('<button>Fire</button>');
	var $timing = $('<input type="number" value="' + timing + '">');
	var $status = $('<td>');
	$row.append($('<td>').text(index));
	$row.append($('<td>').append($value));
	$row.append($('<td>').append($fire));
	$row.append($('<td>').append($timing));
	$row.append($status);
	
	function getValue() {
		return parseInt($value.val());
	};
	function getTiming() {
		return parseInt($timing.val());
	};
	function clear() {
		$status.text("");
	}
	this.value  = getValue;
	this.timing = getTiming;
	this.clear  = clear;

	function fire() {
		var value = getValue();
		clear();
		postCommand (
				{
					method: 'pwmSetPercent',
					params: {
						pwm      : index,
						dutyCycle: value
					}
				},
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
}

var pwmUnits = [];

function init() {
	initPage();

	var $body = $('#pwmTable').find('tbody');
	for (var i = 0; i < 8; i++) {
		var $row = $body.append('<tr>');
		pwmUnits.push(new PwmUnit(i, $row));
	}

	var $sequence = $('#pwmSequence');
	$sequence.click(function(event) {
		for (var i = 0; i < pwmUnits.length; i++) {
			var pwm = pwmUnits[i];
			pwm.clear();
			window.setTimeout(pwm.fire, pwm.timing());
		}
	});
}

jQuery(document).ready(init);
