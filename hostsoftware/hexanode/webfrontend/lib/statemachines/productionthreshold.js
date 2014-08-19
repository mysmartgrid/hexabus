var validator = require('validator');
var common = require('./common');
var validationError = common.validationError;
var StatemachineBuilder = common.StatemachineBuilder;


exports.validateInput = function(msg) {
	if(!msg.hasOwnProperty('producer') || !validator.isIP(msg.producer,6)) {
		console.log("Errror !");
		return new validationError('producer-ip-not-valid', 'The IPv6 address for producer is invalid.');
	}

	if(!msg.hasOwnProperty('productionThreshold') || !validator.isInt(msg.productionThreshold)) {
		return new validationError('production-threshold-invalid', 'The value for production threshold is not a valid integer.');
	}

	if(msg.productionThreshold < 1 || msg.productionThreshold > 4294967295) {
		return new validationError('production-threshold-out-of-range', 'The value for production threshold should be at least 1 and at most 4294967295.');
	}


	if(!msg.hasOwnProperty('offTimeout') || !validator.isInt(msg.offTimeout)) {
		return new validationError('off-timeout-invalid', 'The value for off timeout is not a valid integer.');
	}

	// Max Timeout is 71582788 minutes or 2^32 - 1 seconds
	if(msg.offTimeout < 1 || msg.offTimeout > 71582788) {
		return new validationError('off-timeout-out-of-range', 'The value for off timeout should be at least 1 and at most 71582788.');
	}

	if(!msg.hasOwnProperty('usageThreshold') || !validator.isInt(msg.usageThreshold)) {
		return new validationError('usage-threshold-invalid', 'The value for usage threshold is not a valid integer.');
	}

	if(msg.usageThreshold < 1 || msg.usageThreshold > 4294967295) {
		return new validationError('usage-threshold-out-of-range', 'The value for usage threshold should be at least 1 and at most 4294967295.');
	}


	if(!msg.hasOwnProperty('onTimeout') || !validator.isInt(msg.onTimeout)) {
		return new validationError('on-timeout-invalid', 'The value for on timeout is not a valid integer.');
	}

	// Max Timeout is 71582788 minutes or 2^32 - 1 seconds
	if(msg.onTimeout < 1 || msg.onTimeout > 71582788) {
		return new validationError('on-timeout-out-of-range', 'The value for on timeout should be at least 1 and at most 71582788.');
	}
	
	if(!msg.hasOwnProperty('consumer') || !validator.isIP(msg.consumer,6)) {
		return new validationError('consumer-ip-not-valid', 'The IPv6 address for consumer is invalid.');
	}
};


exports.buildMachine = function(msg, progressCallback, callback) {
	var smb = new StatemachineBuilder();

	console.log('Building productionthreshold');

	smb.onProgress(progressCallback);

	msg.onTimeout = msg.onTimeout * 60;
	msg.offTimeout = msg.offTimeout * 60;

	smb.addTargetFile('productionthreshold/producer.hbh', 'producer.hbh', msg);
	smb.addTargetFile('productionthreshold/consumer.hbh', 'consumer.hbh', msg);
	smb.addTargetFile('productionthreshold/consumer.hbc', 'consumer.hbc', msg);

	smb.addDevice('consumer',true);
	smb.addDevice('producer',false);

	smb.setCompileTarget('consumer.hbc');

	smb.buildStatemachine(function(err) {
		if(!err) {
			callback(true);
		} else {
			console.log(err);
			callback(false, err);
	}});
};
