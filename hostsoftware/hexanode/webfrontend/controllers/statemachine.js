'use strict';

var StatemachineBuilder = require("../lib/statemachines/statemachines").StatemachineBuilder;
var statemachineFromJson = require("../lib/statemachines/statemachines").statemachineFromJson;
var findConflictingStatemachines = require("../lib/statemachines/statemachines").findConflictingStatemachines;

var builder = {};

module.exports.setup = function(devicetree) {
	builder = new StatemachineBuilder(devicetree);
};


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

module.exports.socketioSetup = function(socket, hexabus, devicetree) {

	// Wrapper for socket.on with improved exception handling.
	var on = function(ev, cb) {
		socket.on(ev, function(data, ackcb) {
			try {
				cb(data, ackcb);
			} catch (e) {
				socket.emit('_statemachine_error_', e.toString());
				console.log('Statemachine error: ');
				console.log(e);
				console.log(e.stack);
			}
		});
	};



	on('update_statemachines', function(statemachineJson, cb) {
		console.log(statemachineJson);

		for(var machine in statemachineJson) {
			var statemachine = statemachineFromJson(statemachineJson[machine], devicetree);
			var conflicts = findConflictingStatemachines(statemachine, devicetree);

			if(conflicts.length > 0) {
				cb({success: false, conflicts: conflicts});
				return;
			}

			statemachine.saveToDevicetree();
		}

		console.log('Starting builder');
		builder.build(cb, function(msg) {
			socket.emit('statemachine_progress', msg);
		});
	});


	on('remove_statemachine', function(machineId, cb) {
		devicetree.removeStatemachine(machineId);

		console.log('Starting builder');
		builder.build(cb, function(msg) {
			socket.emit('statemachine_progress', msg);
		});
	});
};
