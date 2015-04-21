'use strict';

var Statemachine = require('./common').Statemachine;
var expectMembers = require('./common').expectMembers;

exports.ProductionThresholdStatemachine = function(id, name, comment, config, devicetree) {
	Statemachine.call(this, id, name, 'ProductionThreshold', comment, config, devicetree);

	expectMembers(this.config, ['producer',
								'prodMeter',
								'prodThreshold',
								'consumer',
								'conMeter',
								'conThreshold',
								'conRelais',
								'conButton',
								'onTimeout',
								'offTimeout']);

	var master = this.getDeviceName(this.config.producer);
	var prodMeter = this.getEndpointName(this.config.producer, this.config.prodMeter);
	var prodThreshold = this.config.prodThreshold;
	var consumer = this.getDeviceName(this.config.consumer);
	var conMeter = this.getEndpointName(this.config.consumer, this.config.conMeter);
	var conThreshold = this.config.conThreshold;
	var conRelais = this.getEndpointName(this.config.consumer, this.config.conRelais);
	var conButton = this.getEndpointName(this.config.consumer, this.config.conButton);
	var onTimeout = this.config.onTimeout * 60;
	var offTimeout = this.config.offTimeout * 60;

	this.getUsedDevices = function() {
		return [this.config.consumer];
	};

	this.getWrittenEndpoints = function() {
		return [this.config.consumer + '.' + this.config.conRelais];
	};

	this.getInstanceLine = function() {



		return this.generateInstanceLine([master,
											prodMeter,
											prodThreshold,
											consumer,
											conMeter,
											conThreshold,
											conRelais,
											conButton,
											onTimeout,
											offTimeout]);
	};
};
exports.ProductionThresholdStatemachine.prototype = Object.create(Statemachine.prototype);
