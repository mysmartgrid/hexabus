'use strict';

var Statemachine = require('./common').Statemachine;
var expectMembers = require('./common').expectMembers;

exports.MasterSlaveStatemachine = function(id, name, comment, config, devicetree) {
	Statemachine.call(this, id, name, 'MasterSlave', comment, config, devicetree);

	expectMembers(this.config, ['master',
								'meter',
								'threshold',
								'slave',
								'relais']);

	var master = this.getDeviceName(this.config.master);
	var slave = this.getDeviceName(this.config.slave);
	var meter = this.getEndpointName(this.config.master, this.config.meter);
	var relais = this.getEndpointName(this.config.slave, this.config.relais);
	var threshold = this.config.threshold;

	this.getUsedDevices = function() {
		return [this.config.slave];
	};

	this.getWrittenEndpoints = function() {
		return [this.config.slave + '.' + this.config.relais];
	};

	this.getInstanceLine = function() {
		return this.generateInstanceLine([master,
											meter,
											threshold,
											slave,
											relais]);
	};
};
exports.MasterSlaveStatemachine.prototype = Object.create(Statemachine.prototype);
