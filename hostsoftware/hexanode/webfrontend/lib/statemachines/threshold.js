'use strict';

var Statemachine = require('./common').Statemachine;

exports.ThresholdStatemachine = function(id, name, comment, config, devicetree) {
	Statemachine.call(this, id, name, 'Threshold', comment, config, devicetree);

	var meterDev = this.getDeviceName(this.config.meterDev);
	var meter = this.getEndpointName(this.config.meterDev, this.config.meter);
	var switchDev = this.getDeviceName(this.config.switchDev);
	var threshold = this.config.threshold;
	var relais = this.getEndpointName(this.config.switchDev, this.config.relais);
	var button = this.getEndpointName(this.config.switchDev, this.config.button);
	var minOnTime = this.config.minOnTime * 60;
	var minOffTime = this.config.minOffTime * 60;

	this.getUsedDevices = function() {
		return [this.config.switchDev];
	};

	this.getWrittenEndpoints = function() {
		return [this.config.switchDev + '.' + this.config.relais];
	};

	this.getInstanceLine = function() {
		return this.generateInstanceLine([meterDev,
											meter,
											threshold,
											switchDev,
											relais,
											button,
											minOnTime,
											minOffTime]);
	};
};
exports.ThresholdStatemachine.prototype = Object.create(Statemachine.prototype);
