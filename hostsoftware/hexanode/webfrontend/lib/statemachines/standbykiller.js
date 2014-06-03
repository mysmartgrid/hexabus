var validator = require('validator');
var common = require('./common');
var validationError = common.validationError;
var StatemachineBuilder = common.StatemachineBuilder;


exports.validateInput = function(msg) {
	if(!msg.hasOwnProperty('threshold') || !validator.isInt(msg.threshold)) {
		return new validationError('threshold-invalid', 'The value for threshold is not a valid integer.');
	}

	if(msg.threshold < 1 || msg.threshold > 4294967295) {
		return new validationError('threshold-nut-of-range', 'The value for threshold should be at least 1 and at most 4294967295.');
	}

	if(!msg.hasOwnProperty('timeout') || !validator.isInt(msg.timeout)) {
		return new validationError('timeout-invalid', 'The value for timeout is not a valid integer.');
	}

	if(msg.timeout < 1 || msg.timeout > 4294967295) {
		return new validationError('timeout-out-of-range', 'The value for timeout should be at least 1 and at most 4294967295.');
	}

	if(!msg.hasOwnProperty('device') || !msg.device.hasOwnProperty('ip') || !validator.isIP(msg.device.ip,6)) {
		return new validationError('device-ip-not-valid', 'The IPv6 address of device is invalid.');
	}
}


exports.buildMachine = function(msg, progressCallback, callback) {
	var smb = new StatemachineBuilder();

	console.log('Building standbykiller');

	smb.onProgress(progressCallback);

	smb.addTargetFile('standbykiller/standbykiller.hbh', 'standbykiller.hbh', { 'ip' : msg.device.ip});
	smb.addTargetFile('standbykiller/standbykiller.hbc', 'standbykiller.hbc', {'threshold' : msg.threshold, 'timeout' : msg.timeout});
	smb.addDevice('standbykiller');

	smb.setCompileTarget('standbykiller.hbc');

	smb.buildStatemachine(function(err) {
		if(!err) {
			callback(true);
		} else {
			console.log(err);
			callback(false, err.toString());
	}});
}
