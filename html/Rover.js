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


// FIXME: this is a copy of XYController - they should share code
function Rover(config) {
	this.getConfig = function() {
		var config = {
				'.class': 'Rover',
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
	addHeaderCell($row, 'Left');
	addHeaderCell($row, 'Right');

	$row = addRow($tbody);
	var $left  = addCell($row);
	var $right = addCell($row);
	$row = addRow($tbody);
	var $fullMotion = $('<input type="checkbox" name="fullMotion" value="1">Full motion</input>');
	addCell($row, $fullMotion, {colspan: 2});

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
		if (tracking) {
			tracking = false;
			sendMovement(0, 0);
		}
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
	
	var motionX;
	var motionY;

	function mouseXY(event) {
		var off = $canvas.offset();
		var mouseX = Math.floor (event.pageX - off.left);
		var mouseY = Math.floor (event.pageY - off.top);
		if (tracking) {
			x = mouseX;
			y = mouseY;
			var scaledX = mouseX / canvas.width;
			var scaledY = (canvas.height - mouseY) / canvas.height;
			sendMovement(
					(scaledX - 0.5) * 200,
					(scaledY - 0.5) * 200
					);
		}
		draw(mouseX, mouseY);
	}

	var timer;

	function sendMovement(scaledX, scaledY) {
		motionX = scaledX;
		motionY = scaledY;

		var full = $fullMotion.is(':checked');
		logPostCommand ({
			method: 'roverMove',
			params: {
				x: scaledX,
				y: scaledY,
				fullMotion: full
			}
		},
		function (result) {
			$left .text((result[0]).toPrecision(3));
			$right.text((result[1]).toPrecision(3));
		}
		);
		if (timer) {
			clearTimeout(timer);
			timer = null;
		}
		if (tracking) {
			timer = setTimeout(function() {
				sendMovement(motionX, motionY);
			}, 800);
		}
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

widgetTypes['Rover'] = Rover;
