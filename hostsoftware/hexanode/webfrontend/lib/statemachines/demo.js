'use strict';

var Statemachine = require('./common').Statemachine;
var expectMembers = require('./common').expectMembers;

exports.DemoPlug = function(name, config, devicetree) {
	Statemachine.call(this, name, 'DemoPlug', config, devicetree);

	expectMembers(this.config, ['device', 'relais', 'button', 'midi', 'buttonPressed', 'onButton', 'offButton']);

	var device = this.getDeviceName(this.config.device);
	var relais = this.getEndpointName(this.config.device, this.config.button);
	var button = this.getEndpointName(this.config.device, this.config.relais);

	var midi = this.getDeviceName(this.config.midi);
	var buttonPressed = this.getEndpointName(this.config.midi, this.config.buttonPressed);

	this.getUsedDevices = function() {
		return [this.config.device];
	};

	this.getWrittenEndpoints = function() {
		return [this.config.device + '.' + this.config.relais];
	};

	this.getInstanceLine = function() {
		return this.generateInstanceLine([device,
											button,
											relais,
											midi,
											buttonPressed,
											this.config.onButton,
											this.config.offButton]);
	};
};
exports.DemoPlug.prototype = Object.create(Statemachine.prototype);
