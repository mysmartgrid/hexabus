'use strict';

var validator = require('validator');
var common = require('./common');
var validationError = common.validationError;
var StatemachineBuilder = common.StatemachineBuilder;

module.exports.validateInput = function(msg) {
	var usedDevices = [];
	
	if(!msg.hasOwnProperty('master') || !msg.master.hasOwnProperty('ip') || !validator.isIP(msg.master.ip,6)) {
		return new validationError('master-ip-not-valid', 'The IPv6 address of master device is invalid.');
	}
	usedDevices.push(msg.master.ip);

	if(!msg.hasOwnProperty('threshold') || !validator.isInt(msg.threshold)) {
		return new validationError('threshold-invalid', 'The value for threshold is not a valid integer.');
	}

	if(msg.threshold < 1 || msg.threshold > 4294967295) {
		return new validationError('threshold-out-of-range', 'The value for threshold should be at least 1 and at most 4294967295.');
	}

	if(!msg.hasOwnProperty('slaves') || msg.slaves.length < 1) {
		return new validationError('no-slaves', 'At least one slave has to be specified');
	}

	console.log(msg.slaves);
	for(var slave in msg.slaves) {
		console.log('Slave ' + slave);
		if(!msg.slaves[slave].hasOwnProperty('ip') || !validator.isIP(msg.slaves[slave].ip,6)) {
			return new validationError('slave-ip-not-valid', 'IPv6 address of slave device is invalid.');
		}
		if(usedDevices.indexOf(msg.slaves[slave].ip) > -1) {
			return new validationError('disjoint-devices', 'One of the specified slave devices has already been in used as slave or master.');
		} 
		console.log('slave' + msg.slaves[slave].ip + 'is okay');
		usedDevices.push(msg.slaves[slave].ip);
	}

	return;
}

module.exports.buildMachine = function(msg, progressCallback, callback) {
		var smb = new StatemachineBuilder();

		smb.onProgress(progressCallback);
		smb.addTargetFile('masterslave/master.hbh', 'master.hbh', { 'masterip' : msg.master.ip});
		smb.addDevice('master',true);

		var slavelist = []
		for(var slave in msg.slaves) {
			var name = 'slave_' + smb.ipToID(msg.slaves[slave].ip);
			console.log(name);
			smb.addTargetFile('masterslave/slave.hbh', name + '.hbh', {'slavename' : name, 'slaveip' : msg.slaves[slave].ip});
			smb.addDevice(name,true);
			slavelist.push(name);
		}


		smb.addTargetFile('masterslave/master_slave.hbc', 'master_slave.hbc', {'threshold' : msg.threshold, 'slaves' : slavelist});
		smb.setCompileTarget('master_slave.hbc');

		smb.buildStatemachine(function(err) {
			if(!err) {
				callback(true);
			} else {
				callback(false, err);
		}});
	}



