'use strict';

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

	this.unpackDeviceGroup = function(group) {
		return group.map(function(dev) { return dev.device; });
	};

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

	this.getDeviceGroup = function(devs) {
		if(devs.length < 1) {
			throw new Error("Could not look up devices names for empty device group");
		}

		devs = this.unpackDeviceGroup(devs);

		var names = devs.map(function(dev) {
			return this.getDeviceName(dev);
		}.bind(this));

		return '[' + names.join(', ') + ']';
	};

	this.getEndpointNameForGroup = function(devs, eid) {
		if(devs.length < 1) {
			throw new Error("Could not look up endpoint name for empty device group");
		}

		devs = this.unpackDeviceGroup(devs);

		var epName = this.getEndpointName(devs[0], eid);
		var allIdentical = devs.every(function(dev) {
			return this.getEndpointName(dev, eid) === epName;
		}.bind(this));

		if(!allIdentical) {
			throw new Error("Found more then one name for eid " + eid);
		}

		return epName;
	}

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


