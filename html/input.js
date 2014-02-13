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

var $configName;

function loadConfig() {
	var name = $configName.val();
	logPostCommand ({
			method: 'readData',
			params: {
				group: 'input',
				name: name
			}
		},
		function (result) {
			print ('Loading: ' + name);
			var config = fromJson(result);
			for (var id in config) {
				var controlConf = config[id];
				var typename = controlConf['.class'];
				var type = controlTypes[typename];
				if (type)
					addControl(type, controlConf);
			}
		}
	);
}

function saveConfig() {
	var name = $configName.val();
	var config = [];
	for (var id in controls) {
		var control = controls[id];
		config.push(control.getConfig());
	}
	logPostCommand ({
			method: 'writeData',
			params: {
				group: 'input',
				name: name,
				data: toJson(config, null, " ")
			}
		},
		function (result) {
			print ('Saved: ' + name);
		}
	);
}

function addControl(type, config) {
	var $div = $('<div>');
	var id = ++nextId;
	var control = new type(
			function() {
				delete controls[id];
				$div.remove();
			},
			config
	);
	controls[id] = control;
	$div.append(control.$widget);
	$div.insertAfter($('#addControl'));
}

function selectControl() {
	var type = $controlSelect.val();
	addControl(controlTypes[type], null);
}

function removeAll() {
	for (item in controls) {
		var control = controls[item];
		control.remove();
		delete controls[item];
	}
}

function init() {
	initPage();

	$('#removeAll').click(removeAll);
	$configName = $('#configName');
	$('#loadConfig').click(loadConfig);
	$('#saveConfig').click(saveConfig);
	$controlSelect = $('#controlSelect');
	for (var key in controlTypes) {
		addOption($controlSelect, key);
	}
	$('#addControl').click(selectControl);
}

jQuery(document).ready(init);
