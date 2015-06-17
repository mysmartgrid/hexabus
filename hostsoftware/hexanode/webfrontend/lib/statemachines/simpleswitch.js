'use strict';

var Statemachine = require('./common').Statemachine;

exports.SimpleSwitchStatemachine = function(id, name, comment, config, devicetree) {
	Statemachine.call(this, id, name, 'SimpleSwitch', comment, config, devicetree);

	var buttonDev = this.getDeviceName(this.config.buttonDev);
	var button = this.getEndpointName(this.config.buttonDev, this.config.button);
	var relaisDev = this.getDeviceName(this.config.relaisDev);
	var relais = this.getEndpointName(this.config.relaisDev, this.config.relais);

	this.getUsedDevices = function() {
		return [this.config.relaisDev];
	};

	this.getWrittenEndpoints = function() {
		return [this.config.relaisDev + '.' + this.config.relais];
	};

	this.getInstanceLine = function() {
		return this.generateInstanceLine([buttonDev,
											button,
											relaisDev,
											relais]);
	};
};
exports.SimpleSwitchStatemachine.prototype = Object.create(Statemachine.prototype);
