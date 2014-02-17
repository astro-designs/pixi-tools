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

var cameraUrl = 'http://' + window.location.hostname + ':9000/?action=';

function CameraStill(config) {
	var attr = {src: cameraUrl + 'snapshot'};
	var $stream = $('<img/>');
	$stream.attr(attr);
	$stream.click(function () {
		$stream.attr(attr);
	});

	this.destroy = function() {
		$stream.attr({src: ''});
	}

	this.$widget = $stream;

	this.getConfig = function() {
		var config = {
				'.class': 'CameraStill'
		};
		return config;
	};
}

widgetTypes['CameraStream'] = CameraStream;

function CameraStream(config) {
	var attr = {src: cameraUrl + 'stream'};
	var $stream = $('<img/>');
	$stream.attr(attr);

	this.destroy = function() {
		$stream.attr({src: ''});
	}

	this.$widget = $stream;

	this.getConfig = function() {
		var config = {
				'.class': 'CameraStream'
		};
		return config;
	};
}

widgetTypes['CameraStream'] = CameraStream;
widgetTypes['CameraStill']  = CameraStill;
