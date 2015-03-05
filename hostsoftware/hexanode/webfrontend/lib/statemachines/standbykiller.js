'use strict';

var Statemachine = require('./common').Statemachine;
var expectMembers = require('./common').expectMembers;
var setMachineClass = require('./common').setMachineClass;

exports.StandbyKillerStatemachine = function(name, config, devicetree) {
	Statemachine.call(this, name, 'Standbykiller', config, devicetree);

	expectMembers(this.config, ['dev',
								'meter',
								'relais',
								'button',
								'threshold',
								'timeout']);

	var dev = this.getDeviceName(this.config.dev);
	var meter = this.getEndpointName(this.config.dev, this.config.meter);
	var relais = this.getEndpointName(this.config.dev, this.config.relais);
	var button = this.getEndpointName(this.config.dev, this.config.button);
	var threshold = this.config.threshold;
	var timeout = this.config.timeout;

	this.getUsedDevices = function() {
		return [this.config.dev];
	};

	this.getWrittenEndpoints = function() {
		return [this.config.dev + '.' + this.config.relais];
	};

	this.getInstanceLine = function() {
		return this.generateInstanceLine([dev,
											meter,
											relais,
											button,
											threshold,
											timeout]);
	};
};
exports.StandbyKillerStatemachine.prototype = Object.create(Statemachine.prototype);
