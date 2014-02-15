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
var $configList;

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
		config.push(controls[id].control.getConfig());
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

function updateConfigs() {
	logPostCommand ({
		method: 'listDataGroup',
		params: {
			group: 'input'
		}
	},
	function (result) {
		setDatalist($configList, result);
	}
);

}

function addControl(type, config) {
	var $div = $('<div>');
	var id = ++nextId;
	var control = new type(
			function() {
			},
			config
	);
	var remove = function() {
		if (control.destroy != null) {
			control.destroy();
		}
		delete controls[id];
		$div.remove();
	};
	controls[id] = {control: control, remove: remove};

	var $left = $('<div style="float:left"/>');
	var $close = $('<button>X</button>');
	$close.click(remove);

	$left.append($close);

	var $right = $('<div>');
	$right.append(control.$widget);

	$div.append($left);
	$div.append($right);
	$div.insertAfter($('#addControl'));
}

function selectControl() {
	var type = $controlSelect.val();
	addControl(controlTypes[type], null);
}

function removeAll() {
	for (item in controls) {
		controls[item].remove();
	}
}

function init() {
	initPage();

	$('#removeAll').click(removeAll);
	$configName = $('#configName');
	$configList = $('#configList');
	$configName.focus(updateConfigs);
	updateConfigs();
	$('#loadConfig').click(loadConfig);
	$('#saveConfig').click(saveConfig);
	$controlSelect = $('#controlSelect');
	for (var key in controlTypes) {
		addOption($controlSelect, key);
	}
	$('#addControl').click(selectControl);
}

jQuery(document).ready(init);
