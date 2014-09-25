'use strict';

var validator = require('validator');
var async = require('async');
var common = require('./common');
var validationError = common.validationError;
var StatemachineBuilder = common.StatemachineBuilder;

module.exports.validateInput = function(msg) {
	return;
};

module.exports.buildMachine = function(msg, progressCallback, callback) {
		var smbLamp = new StatemachineBuilder();

		smbLamp.onProgress(progressCallback);
		smbLamp.addTargetFile('demokoffer/devices.hbh', 'devices.hbh', {});
		smbLamp.addTargetFile('demokoffer/endpoints.hbh', 'endpoints.hbh', {});


		smbLamp.addDevice('Leuchte',true);
		smbLamp.addDevice('Waschmaschine', false);
		smbLamp.addDevice('hexanode', false);
		smbLamp.addDevice('localhost', false);



		smbLamp.addTargetFile('demokoffer/demo_Leuchte.hbc', 'demo_Leuchte.hbc', {});
		smbLamp.setCompileTarget('demo_Leuchte.hbc');



		var smbWashingmachine = new StatemachineBuilder();

		smbWashingmachine.onProgress(progressCallback);
		smbWashingmachine.addTargetFile('demokoffer/devices.hbh', 'devices.hbh', {});
		smbWashingmachine.addTargetFile('demokoffer/endpoints.hbh', 'endpoints.hbh', {});

		smbWashingmachine.addDevice('Waschmaschine',true);		
		smbWashingmachine.addDevice('Leuchte', false);
		smbWashingmachine.addDevice('hexanode', false);
		smbWashingmachine.addDevice('localhost', false);


		smbWashingmachine.addTargetFile('demokoffer/demo_Waschmaschine.hbc', 'demo_Waschmaschine.hbc', {});
		smbWashingmachine.setCompileTarget('demo_Waschmaschine.hbc');

		async.series([smbLamp.buildStatemachine.bind(smbLamp), smbWashingmachine.buildStatemachine.bind(smbWashingmachine)],
					function(error) {
						if(!error) {
							callback(true);
						} else {
							callback(false, error);
						}});
};


