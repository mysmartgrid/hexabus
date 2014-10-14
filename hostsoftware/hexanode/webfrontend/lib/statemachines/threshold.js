var validator = require('validator');
var common = require('./common');
var validationError = common.validationError;
var StatemachineBuilder = common.StatemachineBuilder;


exports.validateInput = function(msg) {
	if(!msg.hasOwnProperty('meterIp') || !validator.isIP(msg.meterIp,6)) {
		return new validationError('meter-ip-not-valid', 'The IPv6 address for meter is invalid.');
	}

	if(!msg.hasOwnProperty('threshold') || !validator.isInt(msg.threshold)) {
		return new validationError('threshold-invalid', 'The value for threshold is not a valid integer.');
	}

	if(msg.threshold < 1 || msg.threshold > 4294967295) {
		return new validationError('threshold-out-of-range', 'The value for threshold should be at least 1 and at most 4294967295.');
	}


	if(!msg.hasOwnProperty('offDelay') || !validator.isInt(msg.offDelay)) {
		return new validationError('off-delay-invalid', 'The value for off delay is not a valid integer.');
	}

	// Max Delay is 71582788 minutes or 2^32 - 1 seconds
	if(msg.offDelay < 1 || msg.offDelay > 71582788) {
		return new validationError('off-delay-out-of-range', 'The value for off delay should be at least 1 and at most 71582788.');
	}

	if(!msg.hasOwnProperty('onDelay') || !validator.isInt(msg.onDelay)) {
		return new validationError('on-delay-invalid', 'The value for on delay is not a valid integer.');
	}

	// Max delay is 71582788 minutes or 2^32 - 1 seconds
	if(msg.onTimeout < 1 || msg.onTimeout > 71582788) {
		return new validationError('on-delay-out-of-range', 'The value for on delay should be at least 1 and at most 71582788.');
	}
	
	if(!msg.hasOwnProperty('switchIp') || !validator.isIP(msg.switchIp,6)) {
		return new validationError('switch-ip-not-valid', 'The IPv6 address for switch is invalid.');
	}
};


exports.buildMachine = function(msg, progressCallback, callback) {
	var smb = new StatemachineBuilder();

	console.log('Building threshold');

	smb.onProgress(progressCallback);

	msg.onDelay = msg.onDelay * 60;
	msg.offDelay = msg.offDelay * 60;

	smb.addTargetFile('threshold/meter.hbh', 'meter.hbh', msg);
	smb.addTargetFile('threshold/switch.hbh', 'switch.hbh', msg);
	smb.addTargetFile('threshold/switch.hbc', 'switch.hbc', msg);

	smb.addDevice('switch',true);
	smb.addDevice('meter',false);

	smb.setCompileTarget('switch.hbc');

	smb.buildStatemachine(function(err) {
		if(!err) {
			callback(true);
		} else {
			console.log(err);
			callback(false, err);
	}});
};
