'use strict';

var Statemachine = require('./common').Statemachine;

exports.MasterSlaveStatemachine = function(id, name, comment, config, devicetree) {
	Statemachine.call(this, id, name, 'MasterSlave', comment, config, devicetree);

	var master = this.getDeviceName(this.config.master);
	var slaves = this.getDeviceGroup(this.config.slaves);
	var meter = this.getEndpointName(this.config.master, this.config.meter);
	var relais = this.getEndpointNameForGroup(this.config.slaves, this.config.relais);
	var threshold = this.config.threshold;

	this.getUsedDevices = function() {
		return this.unpackDeviceGroup(this.config.slaves);
	};

	this.getWrittenEndpoints = function() {
		return this.unpackDeviceGroup(this.config.slaves).map(function(dev) {
			return dev + '.' + this.config.relais;
		}.bind(this));
	};

	this.getInstanceLine = function() {
		return this.generateInstanceLine([master,
											meter,
											threshold,
											slaves,
											relais]);
	};
};
exports.MasterSlaveStatemachine.prototype = Object.create(Statemachine.prototype);
