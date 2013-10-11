'use strict';

var util = require('util');
var EventEmitter = require('events').EventEmitter;
var fs = require('fs');

var unix_ts = function() {
	return Math.round((+new Date()) / 1000);
}

var Device = function(ip, name, emitter, rest) {
	rest = rest || {};

	if (ip == undefined)
		throw "Required parameter: ip";
	if (name == undefined)
		throw "Required parameter: name";

	var last_update = rest.last_update || new Date();
	this.update = function() {
		last_update = new Date();
	};

	Object.defineProperties(this, {
		"ip": {
			value: ip,
			enumerable: true
		},
		"name": {
			enumerable: true,
			get: function() { return name; },
			set: function(val) {
				name = val;
				this.update();
				emitter.emit('device_renamed', device);
			}
		},
		"endpoints": {
			value: {},
			enumerable: true
		},
		"last_update": {
			enumerable: true,
			get: function() { return last_update; }
		}
	});

	this.age = function() {
		return new Date() - this.last_update;
	};

	this.forEachEndpoint = function(cb) {
		for (var key in this.endpoints) {
			if (this.endpoints.hasOwnProperty(key)) {
				cb(this.endpoints[key]);
			}
		}
	};
};



var endpoint_keys = ["type", "description", "function"];
var function_keys = {
	"sensor": [{ name: "minvalue", writable: true }, { name: "maxvalue", writable: true }, "unit"],
	"actor": [],
	"infrastructure": []
};

var endpoint_id = function(ip, eid) {
	if (ip == undefined)
		throw "Required parameter: ip";
	if (eid == undefined)
		throw "Required parameter: eid";

	return ip + "." + eid;
};

var Endpoint = function(device, eid, params, emitter) {
	if (eid == undefined)
		throw "Required parameter: eid";
	eid = parseInt(eid);
	if (isNaN(eid))
		throw "Invalid: eid. Expected: int";

	var last_update = new Date();
	this.update = function(val) {
		last_update = new Date();
		this.device.update();
	};

	var last_value;
	Object.defineProperties(this, {
		"eid": {
			enumerable: true,
			value: eid
		},
		"id": {
			enumerable: true,
			value: endpoint_id(device.ip, eid)
		},
		"ip": {
			enumerable: true,
			value: device.ip
		},
		"device": {
			value: device
		},
		"name": {
			enumerable: true,
			value: device.name
		},
		"last_update": {
			enumerable: true,
			get: function() { return last_update; }
		},
		"last_value": {
			get: function() { return last_value; },
			set: function(val) {
				last_value = val;
				this.update();
				emitter.emit('endpoint_new_value', this);
			}
		}
	});

	var add_key = function(desc) {
		var key, writable;
		if (typeof desc == "object") {
			key = desc.name;
			writable = desc.writable;
		} else {
			key = desc;
			writable = false;
		}

		if (params[key] == undefined)
			throw "Required field: " + key;

		Object.defineProperty(this, key, {
			enumerable: true,
			writable: writable,
			value: params[key]
		});
	};
	endpoint_keys.forEach(add_key.bind(this));
	function_keys[params.function].forEach(add_key.bind(this));

	this.age = function() {
		return new Date() - this.last_update;
	};
};



var DeviceTree = function(file) {
	var devices = {};

	if (file) {
		var data = fs.readFileSync(file);
		var devs_s = JSON.parse(data);
		for (var key in devs_s) {
			var dev_s = devs_s[key];
			var dev = new Device(dev_s.ip, dev_s.name, this, dev_s);

			for (var key_ep in dev_s.endpoints) {
				var ep_s = dev_s.endpoints[key_ep];
				var ep = new Endpoint(dev, key_ep, ep_s, this);
				dev.endpoints[key_ep] = ep;
			}

			devices[key] = dev;
		}
	}

	this.add_endpoint = function(ip, eid, params) {
		var device = devices[ip] || new Device(ip, params.name, this); 
		var endpoint = device.endpoints[eid] || new Endpoint(device, eid, params, this);

		if (!devices[ip]) {
			devices[ip] = device;
			this.emit('device_new', device);
		}

		if (!device.endpoints[eid]) {
			device.endpoints[eid] = endpoint;
			this.emit('endpoint_new', endpoint);
		} else {
			throw "Endpoint exists";
		}

		return endpoint;
	};

	this.forEach = function(cb) {
		for (var key in devices) {
			if (devices.hasOwnProperty(key)) {
				cb(devices[key]);
			}
		}
	};

	this.endpoint_by_id = function(id) {
		var parts = id.split('.');
		var dev = devices[parts[0]];
		if (!dev) {
			return null;
		}
		var ep = dev.endpoints[parts[1]];
		return ep;
	};

	Object.defineProperty(this, "devices", {
		value: devices
	});

	this.save = function(file, cb) {
		fs.writeFile(file, JSON.stringify(devices), function(err) {
			if (cb) {
				cb(err);
			}
		});
	};
};

util.inherits(DeviceTree, EventEmitter);
module.exports = DeviceTree;
