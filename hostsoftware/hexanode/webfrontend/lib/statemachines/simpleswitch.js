'use strict';

var validator = require('validator');
var common = require('./common');
var validationError = common.validationError;
var StatemachineBuilder = common.StatemachineBuilder;

module.exports.validateInput = function(msg) {	
	if(!msg.hasOwnProperty('switchIP') || !validator.isIP(msg.switchIP,6)) {
		return new validationError('switch-ip-not-valid', 'The IPv6 address of switch device is invalid.');
	}

	if(!msg.hasOwnProperty('buttonsOn')) {
		return new validationError('no-buttons-on', 'ButtonsOn member missing in config.');
	}

	if(!msg.hasOwnProperty('buttonsOff')) {
		return new validationError('no-buttons-off', 'ButtonsOff member missing in config.');
	}

	
	var valdiateButtons = function(target) {
		var usedIDs = [];

		for(var index in target) {
			if(!target[index].hasOwnProperty('ip') || !validator.isIP(target[index].ip,6)) {
				return new validationError('button-ip-not-valid', 'IPv6 address of button device is invalid.');
			}
			if(!target[index].hasOwnProperty('eid') || !validator.isInt(target[index].eid)) {
				return new validationError('button-eid-not-valid', 'eid of button device is invalid.');
			}
			if(!target[index].hasOwnProperty('value') || !validator.isInt(target[index].value)) {
				return new validationError('button-value-not-valid', 'value of button device is invalid.');
			}
			if(usedIDs.indexOf(target[index].ip + '.' + target[index].eid) > -1) {
				return new validationError('duplicate-buttons', 'One of the buttons devices has been specified more then once.');
			} 
			usedIDs.push(target[index].ip + '.' + target[index].eid);
		}
	};

	valdiateButtons(msg.buttonsOn);
	valdiateButtons(msg.buttonsOff);

	return;
};

module.exports.buildMachine = function(msg, progressCallback, callback) {
	var smb = new StatemachineBuilder();
	smb.onProgress(progressCallback);


	var endpoints = {};
	var devices = {};

	console.log('foo');

	var updateDevices = function(button) {
		if(devices[button.ip] === undefined) {
			devices[button.ip] = {'name': 'button_' + smb.ipToID(button.ip),
									'ip' : button.ip,
									'endpoints' : [button.eid]};
		}
		else if(devices[button.ip].endpoints.indexOf(button.eid) == -1) {
			devices[button.ip].push(button.eid);
		}
	};

	var updateEndpoints = function(button) {
		if(endpoints[button.eid] === undefined) {
			var newep = { 'eid' : button.eid,
							'name' : 'endpoint' + button.eid,
							'datatype' : 'BOOL'};
			if(button.eid == 25) {
				newep.datatype = 'UINT8';
			}
			endpoints[button.eid] = newep;
		}

	};

	var idsToNames = function(button) {
		var deviceName = devices[button.ip].name;
		if(button.ip == msg.switchIP) {
			deviceName = 'localhost';
		}
		var endpointName = endpoints[button.eid].name;
		var value = button.value;

		return {'deviceName' : deviceName, 'endpointName' : endpointName, 'value' : value};
	};


	var buttonsOn = [];
	for(var index in msg.buttonsOn) {
		updateDevices(msg.buttonsOn[index]);
		updateEndpoints(msg.buttonsOn[index]);
		
		buttonsOn.push(idsToNames(msg.buttonsOn[index]));
	}

	var buttonsOff = [];
	for(index in msg.buttonsOff) {
		updateDevices(msg.buttonsOff[index]);
		updateEndpoints(msg.buttonsOff[index]);

		buttonsOff.push(idsToNames(msg.buttonsOff[index]));
	}

	
	console.log(devices);

	smb.addTargetFile('simpleswitch/devices.hbh', 'devices.hbh', {'devices': devices, 'switchIP': msg.switchIP});
	for(var ip in devices) {
		smb.addDevice(devices[ip].name, false);
	}
	smb.addDevice('SwitchDevice',true);
	smb.addDevice('localhost',false);

	smb.addTargetFile('simpleswitch/endpoints.hbh', 'endpoints.hbh', {'endpoints': endpoints});


	smb.addTargetFile('simpleswitch/simpleswitch.hbc', 'simpleswitch.hbc', {'buttonsOn' : buttonsOn, 'buttonsOff' : buttonsOff});
	smb.setCompileTarget('simpleswitch.hbc');
	

	smb.buildStatemachine(function(err) {
		if(!err) {
			callback(true);
		} else {
			callback(false, err);
	}});

	/*
	var smb = new StatemachineBuilder();

	smb.onProgress(progressCallback);
	smb.addTargetFile('masterslave/master.hbh', 'master.hbh', { 'masterip' : msg.master.ip});
	smb.addDevice('master',true);

	var slavelist = [];
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
	*/
	callback(true);
};



