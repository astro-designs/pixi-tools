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

var $widgetSelect;
var widgets = [];
var nextId = 0;

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
				var widgetConfig = config[id];
				var typename = widgetConfig['.class'];
				var type = widgetTypes[typename];
				if (type)
					addWidget(type, widgetConfig);
			}
		}
	);
}

function saveConfig() {
	var name = $configName.val();
	var config = [];
	for (var id in widgets) {
		config.push(widgets[id].widget.getConfig());
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

function addWidget(type, config) {
	var $div = $('<div>');
	var id = ++nextId;
	var widget = new type(config);
	var remove = function() {
		if (widget.destroy != null) {
			widget.destroy();
		}
		delete widgets[id];
		$div.remove();
	};
	widgets[id] = {widget: widget, remove: remove};

	var $left = $('<div style="float:left"/>');
	var $close = $('<button>X</button>');
	$close.click(remove);

	$left.append($close);

	var $right = $('<div>');
	$right.append(widget.$widget);

	$div.append($left);
	$div.append($right);
	$div.insertAfter($('#addWidget'));
}

function selectWidget() {
	var type = $widgetSelect.val();
	addWidget(widgetTypes[type], null);
}

function removeAll() {
	for (item in widgets) {
		widgets[item].remove();
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
	$widgetSelect = $('#widgetSelect');
	for (var key in widgetTypes) {
		addOption($widgetSelect, key);
	}
	$('#addWidget').click(selectWidget);
}

jQuery(document).ready(init);
