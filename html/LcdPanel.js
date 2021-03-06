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

function LcdPanel(config) {
	var $panel = $('<textarea id="panel" rows="2" cols="40" placeholder="LCD panel text"></textarea>')

	function getText() {
		return $panel.val();
	}
	function sendText() {
		postCommand (
				{
					method: 'lcdSetText',
					params: {
						text: getText()
					}
				}
		);
	}
	$panel.bind('input propertychange', function (event) {
		sendText();
	});
	this.$widget = $panel;

	this.getConfig = function() {
		var config = {
				'.class': 'LcdPanel',
				'text': getText()
		};
		return config;
	};

	if (config) {
		var text = config.text;
		if (text != null)
			$panel.text(text);
	}
}

widgetTypes['LcdPanel'] = LcdPanel;
