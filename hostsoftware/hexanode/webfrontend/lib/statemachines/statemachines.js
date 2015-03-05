'use strict';

var async = require('async');
var fs = require('fs');
var execFile = require('child_process').execFile;
var nconf = require('nconf');

var expectMembers = require('./common').expectMembers;

var SimpleSwitchStatemachine = require('./simpleswitch').SimpleSwitchStatemachine;
var ThresholdStatemachine = require('./threshold').ThresholdStatemachine;
var StandbyKillerStatemachine = require('./standbykiller').StandbyKillerStatemachine;
var MasterSlaveStatemachine = require('./masterslave').MasterSlaveStatemachine;
var ProductionThresholdStatemachine = require('./productionthreshold').ProductionThresholdStatemachine;


var machineTypes = {
	'SimpleSwitch' : SimpleSwitchStatemachine,
	'Threshold' : ThresholdStatemachine,
	'Standbykiller' : StandbyKillerStatemachine,
	'MasterSlave' : MasterSlaveStatemachine,
	'ProductionThreshold' : ProductionThresholdStatemachine
};


var statemachineFromDevicetree = function(name, devicetree) {

	if(devicetree === undefined) {
		throw new Error("devicetree parameter is missing");
	}

	if(devicetree.statemachines[name] === undefined) {
		throw new Error("Statemachine " + name + " does not exist in devicetree");
	}

	var machineClass = devicetree.statemachines[name].machineClass;
	if(machineTypes[machineClass] === undefined) {
		throw new Error("Unknown machine class " + machineClass);
	}

	var config = devicetree.statemachines[name].config;

	return new machineTypes[machineClass](name, config, devicetree);
};

exports.statemachineFromJson = function(json, devicetree) {
	expectMembers(json, ['name',
						'machineClass',
						'config']);

	if(devicetree === undefined) {
		throw new Error("devicetree parameter is missing");
	}

	if(machineTypes[json.machineClass] === undefined) {
		throw new Error("Unknown machine class " + json.machineClass);
	}

	return new machineTypes[json.machineClass](json.name, json.config, devicetree);
};


exports.findConflictingStatemachines = function(statemachine, devicetree) {
	var conflicts = [];
	var endpoints  = statemachine.getWrittenEndpoints();

	for(var name in devicetree.statemachines) {
		var otherEndpoints = statemachineFromDevicetree(name, devicetree).getWrittenEndpoints();
		for(var index in endpoints) {
			if(otherEndpoints.indexOf(endpoints[index]) !== -1) {
				conflicts.push(name);
			}
		}
	}
	return conflicts;
};


exports.StatemachineBuilder = function(devicetree) {
	var buildDir = 'statemachines/build/';
	var instanceFile = buildDir + '/instances.hbc';

	var progressCallback = console.log;

	var statemachines = [];
	var deviceIps = [];

	var localizeError = function(localization, error, extras) {
		extras = extras || {};
		return {'msg': error.toString(), 'localization': localization, 'extras' : extras};
	};


	var setProgress = function(localization, extras) {
		extras = extras || {};
		progressCallback({'localization' : localization, 'extras' : extras});
	};


	var collectMachines = function(callback) {
		statemachines = [];

		setProgress('collecting-machines');

		try {
			for(var machineName in devicetree.statemachines) {
				statemachines.push(statemachineFromDevicetree(machineName, devicetree));
			}
			callback();
		}
		catch(error) {
			callback(localizeError('collecting-machines-failed', error));
		}
	};

	var collectDevices = function(callback) {

		setProgress('collecting-devices');

		var hashSet = {};
		for(var machineIndex in statemachines) {
			var devices = statemachines[machineIndex].getUsedDevices();
			for(var devIndex in devices) {
				hashSet[devices[devIndex]] = true;
			}
		}

		deviceIps = Object.keys(hashSet);
		callback();
	};

	var generateInstanceFile = function(callback) {

		var instanceLines = statemachines.map(function(statemachine, index, machines) {
			setProgress('generating-instance', {'machine' : statemachine.name,
												'count' : machines.length,
												'done' : index + 1});
			return statemachine.getInstanceLine();
		});

		try {
			var header = fs.readFileSync('statemachines/instances.head');
			var instances = header + instanceLines.join('\n');
			fs.writeFileSync(instanceFile, instances);
			callback();
		}
		catch(error) {
			callback(localizeError('creating-intance-file-failed', error));
		}
	};

	var compileStatemachines = function(callback) {
		setProgress('compiling-statemachines');
		execFile('hbc',['-o', buildDir + '/%d', instanceFile], function(error, stdout, stderr) {
			if(error) {
				console.log(error);
				callback(localizeError('compiling-failed', stdout));
			}
			else {
				callback();
			}
		});
	};


	var uploadStatemachines = function(callback) {
		var uploadStatemachine = function(ip, callback) {
			console.log(buildDir + '/' + devicetree.devices[ip].sm_name);
			setProgress('uploading',  { 'device' : devicetree.devices[ip].sm_name, 
										'done' : deviceIps.indexOf(ip) + 1,
										'count' : deviceIps.length});
			execFile('hexaupload',
				['--ip', ip, '--program', buildDir + '/' + devicetree.devices[ip].sm_name], 
				function(error, stdout, stderr) {
					if(error) {
						callback(localizeError('uploading-failed', stdout, {'device': devicetree.devices[ip].name}));
					}
					else {
						callback();
					}
			});
		};

		async.mapSeries(deviceIps, uploadStatemachine, callback);
	};


	var cleanupFiles = function(callback) {
		setProgress('deleting-temporary-files');
		var deleteFile = function(file, callback) {
			fs.unlink(file, function(error) {
				if(error && error.code !== 'ENOENT') {
					callback(localizeError('deleting-temporary-files-failed', error, {'file': file}));	
				}
				else {
					callback();
				}
			});
		};

		var devFiles = deviceIps.map(function(ip) { return devicetree.devices[ip].sm_name; });

		async.map(devFiles, deleteFile, callback);
	};

	var busy = false;

	this.isBusy = function() {
		return busy;
	};

	this.build = function(callback, progressCb) {
		if(busy) {
			callback(localizeError('statemachinebuilder-busy', 'Statemachine builder is already running'));
			return;
		}

		busy = true;

		progressCallback = progressCb || console.log;
		async.series([collectMachines,
			collectDevices,
			generateInstanceFile,
			compileStatemachines,
			uploadStatemachines,
			cleanupFiles],
			function(error) {
				if(error) {
					callback({'success' : false, 'error' : error});
				}
				else {
					callback({'success' : true});
				}
				busy = false;
			});
	};
};
