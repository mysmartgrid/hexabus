'use strict';

var statemachines = require("../lib/statemachines");

module.exports.expressSetup = function(app, nconf, hexabus, devicetree) {
	app.get('/statemachine', function(req, res) {
		var devices = {};

		devicetree.forEach(function(device) {
			var entry = devices[device.ip] = { name: device.name, ip: device.ip, eids: [] };

			device.forEachEndpoint(function(ep) {
				if (ep.function != "infrastructure") {
					entry.eids.push(ep);
				}
			});
			entry.eids.sort(function(a, b) {
				return b.eid - a.eid;
			});
		});

		res.render('statemachine/statemachine.ejs', { active_nav: 'statemachines', devices: devices });
	});
};

module.exports.socketioSetup = function(on, emit, hexabus, devicetree) {
	

	var busy = false;

	var sendError = function(error) {
		emit('sm_uploaded', {sucess: false, error: error});
	};

	var buildStatemachine = function(msg, statemachineModule) {
		console.log(msg);
		if(!busy) {			
			console.log("Validating data");

			var error = statemachineModule.validateInput(msg);
			console.log(error);
			if(error) {
				console.log("Validation failed");
				console.log(error);
				sendError(error);
				return;
			}

			console.log("Data passed validation");

			busy = true;
			statemachineModule.buildMachine(msg,
				function(msg) {
					emit('sm_progress', msg);
				},
				function(success, error) {
					console.log(error);
					busy = false;
					emit('sm_uploaded', {success: success, error: error});
			});
		}
	};

	on('master_slave_sm', function(msg) {
		buildStatemachine(msg,statemachines.masterSlave);
	});

	on('standbykiller_sm', function(msg) {
		buildStatemachine(msg,statemachines.standbyKiller);
	});

	on('productionthreshold_sm', function(msg) {
		buildStatemachine(msg,statemachines.productionThreshold);
	});
};
