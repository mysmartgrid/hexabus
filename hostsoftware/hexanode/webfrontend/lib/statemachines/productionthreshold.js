var validator = require('validator');
var common = require('./common');
var validationError = common.validationError;
var StatemachineBuilder = common.StatemachineBuilder;


exports.validateInput = function(msg) {
	if(!msg.hasOwnProperty('source') || !validator.isIP(msg.source,6)) {
		console.log("Errror !");
		return new validationError('soruce-ip-not-valid', 'The IPv6 address for source is invalid.');
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

	if(msg.offTimeout < 1 || msg.offTimeout > 4294967295) {
		return new validationError('off-timeout-out-of-range', 'The value for off timeout should be at least 1 and at most 4294967295.');
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

	if(msg.onTimeout < 1 || msg.onTimeout > 4294967295) {
		return new validationError('on-timeout-out-of-range', 'The value for on timeout should be at least 1 and at most 4294967295.');
	}
	
	if(!msg.hasOwnProperty('switchDevice') || !validator.isIP(msg.switchDevice,6)) {
		return new validationError('soruce-ip-not-valid', 'The IPv6 address for switch is invalid.');
	}
}


exports.buildMachine = function(msg, progressCallback, callback) {
	var smb = new StatemachineBuilder();

	console.log('Building productionthreshold');

	smb.onProgress(progressCallback);

	smb.addTargetFile('productionthreshold/source.hbh', 'source.hbh', msg);
	smb.addTargetFile('productionthreshold/fridge.hbh', 'fridge.hbh', msg);
	smb.addTargetFile('productionthreshold/fridge.hbc', 'fridge.hbc', msg);

	smb.addDevice('fridge',true);
	smb.addDevice('source',false);

	smb.setCompileTarget('fridge.hbc');

	smb.buildStatemachine(function(err) {
		if(!err) {
			callback(true);
		} else {
			console.log(err);
			callback(false, err);
	}});
}
