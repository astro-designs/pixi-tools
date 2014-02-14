/*
    pixi-tools: a set of software to interface with the Raspberry Pi
    and PiXi-200 hardware
    Copyright (C) 2014 Simon Cantrill

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

function Controller($row, config) {
	var attr = {valign: 'top'};
	var number = '<input type="number"/>';
	this.$pin     = $(number);
	this.$min     = $(number);
	this.$max     = $(number);
	addCell($row, this.$pin);
	addCell($row, this.$min);
	addCell($row, this.$max);
	this.$value   = addCell($row, '0');

	this.$pin  .val(config['pin']);
	this.$min  .val(config['min']);
	this.$max  .val(config['max']);
	this.$value.text(config['value']);

	this.update = function(scaled) {
		var pin     = parseInt(this.$pin.val());
		var min     = parseInt(this.$min.val());
		var max     = parseInt(this.$max.val());
		var value = Math.round (min + (scaled * (max - min)));
		if (value == this.value)
			return;

		this.value = value;
		var $value = this.$value;

		logPostCommand (
				{
					method: 'pwmWritePin',
					params: {
						pwm:       pin,
						dutyCycle: value
					}
				},
				function (result) {
					$value.text(value);
				},
				function (error) {
					$value.text(value + ': ' + error);
				}
		);
	};
}

function XYController(config) {
	this.getConfig = function() {
		var config = {
				'.class': 'XYController',
		};
		return config;
	};

	var tracking = false;

	var x;
	var y;

	var $table = $('<table>');
	this.$widget = $table;
	var $tbody = $('<tbody>');
	$table.append($tbody);
	var $row = addRow($tbody);
	var $div = $('<div class="controller"/>');
	var $canvas = $('<canvas width="200" height="200"/>');
	$div.append($canvas);

	addCell($row, $div, {rowspan: 3});
	addHeaderCell($row, 'PWM pin');
	addHeaderCell($row, 'Minimum');
	addHeaderCell($row, 'Maximum');
	addHeaderCell($row, 'Value');

	var pinConf = {
			pin: 0,
			min: 60,
			max: 70,
			value: 65
	};
	$row = addRow($tbody);
	var controllerX = new Controller($row, pinConf);

	pinConf['pin'] = 1;
	$row = addRow($tbody);
	var controllerY = new Controller($row, pinConf);

	var canvas = $canvas[0];
	if (canvas != null && canvas.getContext) {
		var context = canvas.getContext('2d');
		if (context == null)
			return;
	}
	var gradientX = context.createLinearGradient(0, 0, canvas.width, 0);
	gradientX.addColorStop(0, "rgba(64, 64, 64, 0.3)");
	gradientX.addColorStop(1, "rgba(255, 255, 255, 0.3)");
	var gradientY = context.createLinearGradient(0, canvas.height, 0, 0);
	gradientY.addColorStop(0, "rgba(64, 64, 64, 0.3)");
	gradientY.addColorStop(1, "rgba(255, 255, 255, 0.3)");

	drawGrid(canvas, context);

	$canvas.mousemove(function(event) {
		mouseXY(event);
	});
	$canvas.mousedown(function(event) {
		tracking = true;
		mouseXY(event);
	});
	var stop = function(event) { 
		tracking = false;
		draw(x, y);
	};
	$canvas.mouseup(stop);
	$canvas.mouseleave(stop);

	function drawGrid(canvas, context) {
		context.beginPath();
		context.rect(0, 0, canvas.width, canvas.height);
		context.fillStyle = "#101010";
		context.fill();

		context.lineWidth = 1;
		context.strokeStyle = "#202020";
		for ( var i = 0; i < canvas.width; i += 20) {
			context.beginPath();
			context.moveTo(i + 0.5, 0);
			context.lineTo(i + 0.5, canvas.height);
			context.stroke();
		}
		for ( var i = 0; i < canvas.height; i += 20) {
			context.beginPath();
			context.moveTo(0, i + 0.5);
			context.lineTo(canvas.width, i + 0.5);
			context.stroke();
		}
	}

	function mouseXY(event) {
		var off = $canvas.offset();
		var mouseX = Math.floor (event.pageX - off.left);
		var mouseY = Math.floor (event.pageY - off.top);
		if (tracking) {
			x = mouseX;
			y = mouseY;
			controllerX.update (mouseX / canvas.width);
			controllerY.update ((canvas.height - mouseY) / canvas.height);
		}
		draw(mouseX, mouseY);
	}

	function draw(mouseX, mouseY) {
		// FIXME: for efficiency (particularly with firefox),
		// should paint only the changed areas
//		canvas = document.getElementById('canvas');
//		context = canvas.getContext('2d');
		drawGrid(canvas, context);

		context.beginPath();
		context.moveTo(0, 0);
		context.rect(0, 0, x, canvas.height);
		context.fillStyle = gradientX;
		context.fill();

		context.beginPath();
		context.moveTo(0, 0);
		context.rect(0, y, canvas.width, canvas.height);
		context.fillStyle = gradientY;
		context.fill();

		context.beginPath();
		context.moveTo (0           , mouseY + 0.5);
		context.lineTo (canvas.width, mouseY + 0.5);
		context.moveTo (mouseX + 0.5, 0            );
		context.lineTo (mouseX + 0.5, canvas.height);

		context.lineWidth = 1;
		context.strokeStyle = "rgba(255, 255, 255, 0.5)";
		context.stroke();
	}
}

controlTypes['XYController'] = XYController;
