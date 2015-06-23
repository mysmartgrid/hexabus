'use strict';

var async = require('async');
var fs = require('fs');
var tv4 = require('tv4');
var execFile = require('child_process').execFile;
var nconf = require('nconf');
var path = require('path');
var uuid = require('node-uuid');

var debug = require('../debug.js');

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


var jsonSchemas = {};
for(var type in machineTypes) {
	jsonSchemas[type] = require('./schemas/' + type + '.json');
}


var statemachineFromDevicetree = function(id, devicetree) {

	if(devicetree === undefined) {
		throw new Error("devicetree parameter is missing");
	}

	if(devicetree.statemachines[id] === undefined) {
		throw new Error("Statemachine " + id + " does not exist in devicetree");
	}

	var machineClass = devicetree.statemachines[id].machineClass;
	if(machineTypes[machineClass] === undefined) {
		throw new Error("Unknown machine class " + machineClass);
	}

	var machine = devicetree.statemachines[id];

	return new machineTypes[machineClass](id, machine.name, machine.comment, machine.config, devicetree);
};
exports.statemachineFromDevicetree = statemachineFromDevicetree;


exports.statemachineFromJson = function(json, devicetree) {

	if(devicetree === undefined) {
		throw new Error("devicetree parameter is missing");
	}

	if(machineTypes[json.machineClass] === undefined) {
		throw new Error("Unknown machine class " + json.machineClass);
	}

	if(json.id === null) {
		json.id = uuid.v4();
	}

	if(!tv4.validate(json, jsonSchemas[json.machineClass])) {
		throw new Error("Valitation of json failed at " + tv4.error.dataPath + " with: " + tv4.error.message); 
	}

	return new machineTypes[json.machineClass](json.id, json.name, json.comment, json.config, devicetree);
};


exports.findConflictingStatemachines = function(statemachine, devicetree) {
	var conflicts = [];
	var endpoints  = statemachine.getWrittenEndpoints();

	for(var id in devicetree.statemachines) {
		var otherEndpoints = statemachineFromDevicetree(id, devicetree).getWrittenEndpoints();
		for(var index in endpoints) {
			if(otherEndpoints.indexOf(endpoints[index]) !== -1 && conflicts.indexOf(id) === -1) {
				conflicts.push(id);
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
		var localError = {'msg': error.toString(), 'localization': localization, 'extras' : extras};
		debug(localError);
		return localError;
	};


	var setProgress = function(localization, extras) {
		extras = extras || {};
		progressCallback({'localization' : localization, 'extras' : extras});
	};


	var checkHeaders = function(callback) {
		if(!fs.existsSync(path.join(nconf.get("data"), 'endpoints.hbh')) || !fs.existsSync(path.join(nconf.get("data"), 'devices.hbh'))) {
			callback(localizeError('headers-missing', 'devices.hbh and endpoints.hbh are missing.'));
		}
		else {
			callback();
		}
	};

	var collectMachines = function(callback) {
		statemachines = [];

		setProgress('collecting-statemachines');

		try {
			for(var machineId in devicetree.statemachines) {
				statemachines.push(statemachineFromDevicetree(machineId, devicetree));
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
			if(!fs.existsSync(buildDir)) {
				fs.mkdir(buildDir);
			}

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

		//var devFiles = deviceIps.map(function(ip) { return devicetree.devices[ip].sm_name; });

		//async.map(devFiles, deleteFile, callback);
		callback();
	};

	var busy = false;

	this.isBusy = function() {
		return busy;
	};

	this.build = function(callback, progressCb) {
		if(busy) {
			callback({'success' : false, 'error': localizeError('statemachinebuilder-busy', 'Statemachine builder is already running')});
			return;
		}

		busy = true;

		progressCallback = progressCb || console.log;
		async.series([
			checkHeaders,
			collectMachines,
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
