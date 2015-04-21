'use strict';



exports.expectMembers = function(obj, members) {
	console.log(obj);
	for(var index in members) {
		if(obj[members[index]] === undefined) {
			console.trace();
			throw new Error("Expected member " + members[index]);
		}
	}
};

exports.Statemachine = function(id, name, machineClass, comment, config, devicetree) {

	Object.defineProperties(this, {
		'id': {
			get: function() { return id; },
			enumerable: true
		},

		'name': {
			get: function() { return name; },
			enumerable: true
		},
		'config': {
			get: function() { return config; },
			enumerable: true
		},
		'machineClass' : {
			enumarable: true,
			get: function(){
				return machineClass;
			}
		}
	});

	this.getDeviceName = function(ip) {
		if(devicetree.devices[ip] === undefined) {
			throw new Error("Unknown device " + ip);
		}
		return devicetree.devices[ip].sm_name;
	};

	this.getEndpointName = function(ip, eid) {
		if(devicetree.devices[ip] === undefined) {
			throw new Error("Unknown device " + ip);
		}

		if(devicetree.devices[ip].endpoints[eid] === undefined) {
			throw new Error("Unknown eid " + eid + " for device " + ip);
		}

		return 	devicetree.devices[ip].endpoints[eid].sm_name;
	};

	this.getUsedDevices = function() {
		throw new Error("Missing implementation for getUsedDevices");
	};

	this.getWrittenEndpoints = function() {
		throw new Error("Missing implementation for getWrittenEndpoints");
	};


	this.generateInstanceLine = function(parameters) {
		var id = 'sm_' + this.id.replace(/-/g,'_');
		var parameterList = parameters.join(', ');
		return 'machine ' + id + ': ' + this.machineClass + '(' + parameterList + ');';
	};

	this.getInstanceLine = function() {
		throw new Error("Missing implementation for getInstance");
	};

	this.saveToDevicetree = function() {
		if(devicetree.statemachines[id] === undefined) {
			devicetree.addStatemachine(name, this.machineClass, comment, config);
		}
		else {
			devicetree.statemachines[id].updateConfig(config);
		}
	};
};


