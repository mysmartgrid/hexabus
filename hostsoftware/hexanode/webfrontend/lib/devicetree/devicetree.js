'use strict';

var util = require('util');
var EventEmitter = require('events').EventEmitter;
var fs = require('fs');

var unix_ts = function() {
	return Math.round((+new Date()) / 1000);
}

var Device = function(ip, name, emitter, rest) {
	var endpoints = {};
	var last_update;
	var name;

	this.toJSON = function() {
		return {
			ip: ip,
			name: name,
			last_update: last_update,
			endpoints: endpoints
		};
	};

	Object.defineProperties(this, {
		"ip": {
			get: function() { return ip; },
			enumerable: true
		},
		"name": {
			enumerable: true,
			get: function() { return name; },
			set: function(val) {
				name = val;
				this.update();
				emitter.emit('device_renamed', this);
			}
		},
		"endpoints": {
			value: endpoints,
			enumerable: true
		},
		"last_update": {
			enumerable: true,
			get: function() { return last_update; }
		},
		"age": {
			get: function() { return new Date() - this.last_update; }
		}
	});

	this.update = function() {
		last_update = new Date();
	};

	this.forEachEndpoint = function(cb) {
		for (var key in this.endpoints) {
			if (this.endpoints.hasOwnProperty(key)) {
				cb(this.endpoints[key]);
			}
		}
	};

	if (arguments.length == 1) {
		var obj = ip;

		emitter = obj.emitter;

		ip = obj.dev.ip;
		name = obj.dev.name;
		last_update = new Date(obj.dev.last_update);

		for (var ep_id in obj.dev.endpoints) {
			endpoints[ep_id] = new Endpoint({
				device: this,
				emitter: emitter,
				ep: obj.dev.endpoints[ep_id]
			});
		}
	} else {
		rest = rest || {};

		last_update = rest.last_update || new Date();
	}

	if (ip == undefined)
		throw "Required parameter: ip";
	if (name == undefined)
		throw "Required parameter: name";
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
	var last_update;
	var last_value;

	this.update = function(val) {
		last_update = new Date();
		this.device.update();
	};

	Object.defineProperties(this, {
		"eid": {
			enumerable: true,
			get: function() { return eid; }
		},
		"id": {
			enumerable: true,
			get: function() { return endpoint_id(device.ip, eid); }
		},
		"ip": {
			enumerable: true,
			get: function() { return device.ip; }
		},
		"device": {
			get: function() { return device; }
		},
		"name": {
			enumerable: true,
			get: function() { return device.name; }
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
			},
		},
		"age": {
			get: function() {
				return new Date() - this.last_update;
			}
		}
	});

	var add_key = function(to, desc) {
		var key, writable;
		if (typeof desc == "object") {
			key = desc.name;
			writable = desc.writable;
		} else {
			key = desc;
			writable = false;
		}

		if(key == "type" && typeof params[key] != "number") {
			console.trace("Field type is:" + params[key]);
			throw "Field type is supposed to be a number.";
		}

		if (params[key] == undefined) {
			throw "Required field: " + key;
		}

		Object.defineProperty(to, key, {
			enumerable: true,
			writable: writable,
			value: params[key]
		});
	};

	var add_function_keys = function(to) {
		var add = function(key) {
			add_key(to, key);
		};
		endpoint_keys.forEach(add);
		function_keys[params.function].forEach(add);
	};

	this.toJSON = function() {
		var json = {
			last_value: this.last_value
		};

		for (var key in this) {
			json[key] = this[key];
		}

		return json;
	};

	var set_minmax_values = function() {
		//FIXME: Load min max values from config file
		switch (params.eid) {
		case 2: // power meter
			params.minvalue = 0;
			params.maxvalue = 3600;
			break;

		case 3: // temperature
			params.minvalue = 10;
			params.maxvalue = 35;
			break;

		case 5: // humidity sensor
			params.minvalue = 0;
			params.maxvalue = 100;
			break;

		case 44: // pt100 sensors for heater inflow/outflow
		case 45:
			params.minvalue = 0;
			params.maxvalue = 100;
			break;

		default:
			params.minvalue = 0;
			params.maxvalue = 100;
			break;
		}
	}

	if (arguments.length == 1) {
		var obj = device;

		device = obj.device;
		emitter = obj.emitter;

		eid = obj.ep.eid;
		last_update = new Date(obj.ep.last_update);
		last_value = obj.ep.last_value;
		params = obj.ep;
	} else {
		last_update = new Date();
	}

	if (eid == undefined)
		throw "Required parameter: eid";
	eid = parseInt(eid);
	if (isNaN(eid))
		throw "Invalid: eid. Expected: int";

	set_minmax_values();

	add_function_keys(this);
};



var DeviceTree = function(file) {
	var devices = {};
	var views = {};

	if (file) {
		var data = fs.readFileSync(file);
		var json = JSON.parse(data);

		for (var key in json.devices) {
			devices[key] = new Device({
				emitter: this,
				dev: json.devices[key]
			});
		}
		views = json.views || {};
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

	this.remove = function(ip) {
		if (devices[ip]) {
			this.emit('device_remove', devices[ip]);
			delete devices[ip];
		} else {
			throw "No such device";
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

	Object.defineProperties(this, {
		devices: {
			value: devices
		},
		views: {
			value: views
		}
	});

	this.toJSON = function() {
		var json = {
			devices: devices,
			views: views
		};
		return json;
	}

	this.save = function(file, cb) {
		var json = {
			devices: devices,
			views: views
		};
		fs.writeFile(file, JSON.stringify(json, null, '\t'), function(err) {
			if (cb) {
				cb(err);
			}
		});
	};
};

util.inherits(DeviceTree, EventEmitter);
module.exports = DeviceTree;
