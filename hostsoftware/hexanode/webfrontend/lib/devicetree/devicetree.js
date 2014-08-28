'use strict';

var util = require('util');
var EventEmitter = require('events').EventEmitter;
var fs = require('fs');
var uuid = require('node-uuid');

/*
 *  Helper function to get seconds since the epoch (instead of milliseconds)
 */
var unix_ts = function() {
	return Math.round(new Date() / 1000);
};

var isEmptyObject = function(obj) {
	return (Object.keys(obj).length === 0);
};

/*
 * Constructor for objects representing a hexabus device
 * 
 * This Constructor can be used in two ways
 *	1. new Device(ip, name, emitter, rest)
 *  2. new Device(jsonRepresentation)
 *
 * Members:
 * ip 					ipv6 address of the device
 * name 				name of the device (a provided by hexinfo)
 * endpoints			list of endpoints for this device (see Endpoint constructor)
 * last_update			unix timestamp of the last time update() was called
 * age					time in seconds since last_update
 * to_JSON()			generates a simplfied version of the instance for json serialization
 * update()				set the last_update to the current timestamp.
 *						Called evertime the device is renamed or a new value is set for one
 *						of its endpoints.
 *						May also be called externally e.g. if hexinfo has seen the device.
 * forEachEndpoint(cb)	Calls cb(endpoint) for each endpoint of the device	
 */ 
var Device = function(ip, name, emitter, last_update) {
	var endpoints = {};

	this.propagateUpdate = function(endpointUpdate) {	
		var update = {};
		update.devices = {};

		if(endpointUpdate === undefined) {
			update.devices[ip] = {
				'name': name,
				'last_update' : last_update
			};
		}
		else {
			update.devices[ip] = {
				'last_update' : last_update,
				'endpoints' : endpointUpdate
			};
		}

		emitter.propagateUpdate(update);
	};

	this.applyUpdate = function(update) {
		if(update.name !== undefined && update.last_update !== undefined) {
			name = update.name;
			last_update = update.last_update;
		}

		if(update.endpoints !== undefined) {
			for(var eid in update.endpoints) {
				if(endpoints[eid] !== undefined) {
					endpoints[eid].applyUpdate(update.endpoints[eid]);
				}
				else {
					var endpointUpdate = update.endpoints[eid];
					if(endpointUpdate.params === undefined) {
						throw "Can not create endpoint without params";
					}
					endpoints[eid] = new Endpoint(this, 
													eid, 
													endpointUpdate.params, 
													emitter,
													endpointUpdate.last_value, 
													endpointUpdate.last_update);
				}
			}
		}
	};

	this.propagateDeletion = function(childDeletion) {
		var deletion = {};
		deletion.devices = {};
		deletion.devices[ip] = childDeletion;
		emitter.propagateDeletion(deletion);
	};

	this.propagteEnpointDeletion = function(eid) {
		var deletion = {};
		deletion.endpoints = {};
		deletion.endpoints[eid] = {};
		this.propagateDeletion(deletion);
	};

	this.applyDeletion = function(deletion) {
		for(var eid in deletion.endpoints) {
			if(isEmptyObject(deletion.endpoints[eid])) {
				delete endpoints[eid];
			}
		}
	};

	this.addEndpoint = function(eid, params) {
		if(endpoints[eid] !== undefined) {
			throw "Endpoint already exists";
		}

		var endpoint = new Endpoint(this, eid, params, emitter);
		endpoints[eid] = endpoint;

		endpoint.propagateCreation(params);

		return endpoint;
	};

	this.removeEndpoint = function(eid) {
		if(endpoints[eid] === undefined) {
			throw "Endpoint not found";
		}

		delete endpoints[eid];
		this.propagteEnpointDeletion(eid);
	};

	Object.defineProperties(this, {
		'ip': {
			get: function() { return ip; },
			enumerable: true
		},
		'name': {
			enumerable: true,
			get: function() { return name; },
			set: function(val) {
				name = val;
				//takes also care of calling propagateUpdate
				this.update();
				emitter.emit('device_renamed', this);
			}
		},
		'endpoints': {
			value: endpoints,
			enumerable: true
		},
		'last_update': {
			enumerable: true,
			get: function() { return last_update; }
		},
		'age': {
			get: function() { return unix_ts() - this.last_update; }
		}
	});

	this.toJSON = function() {
		return {
			'ip': ip,
			'name': name,
			'last_update': last_update,
			'endpoints': endpoints
		};
	};

	this.update = function() {
		last_update = unix_ts();
		this.propagateUpdate();
	};

	this.forEachEndpoint = function(cb) {
		for (var key in this.endpoints) {
			if (this.endpoints.hasOwnProperty(key)) {
				cb(this.endpoints[key]);
			}
		}
	};

	// Check if we've been called using only a json representation
	if (arguments.length == 1) {
		var obj = ip;

		emitter = obj.emitter;

		ip = obj.dev.ip;
		name = obj.dev.name;
		last_update = obj.dev.last_update;

		for (var ep_id in obj.dev.endpoints) {
			endpoints[ep_id] = new Endpoint({
				device: this,
				emitter: emitter,
				ep: obj.dev.endpoints[ep_id]
			});
		}
	// We've been called with a normal set of parameters
	} else {
		last_update = last_update || unix_ts();
	}

	if (ip === undefined)
		throw "Required parameter: ip";
	if (name === undefined)
		throw "Required parameter: name";
};



var endpoint_id = function(ip, eid) {
	if (ip === undefined)
		throw "Required parameter: ip";
	if (eid === undefined)
		throw "Required parameter: eid";

	return ip + "." + eid;
};

var Endpoint = function(device, eid, params, emitter, last_value, last_update) {

	this.propagateUpdate = function() {
		var update = {};
		update[eid] = {
			'last_update': last_update,
			'last_value' : last_value,
		};

		if(this.function == "sensor") {
			update[eid].maxvalue = this.maxvalue;
			update[eid].minvalue = this.minvalue;
		}

		device.propagateUpdate(update);
	};

	this.propagateCreation = function() {
		var update = {};
		update[eid] = {
			'last_update': last_update,
			'last_value' : last_value,
			'params' : params
		};

		device.propagateUpdate(update);
	};

	this.applyUpdate = function(update) {
		if(update.last_update !== undefined && update.last_value !== undefined) {
			last_update = update.last_update;
			last_value = update.last_value;
		}

		if(this.function == "sensor" && 
			update.minvalue !== undefined && update.maxvalue !== undefined) {
			params.minvalue = update.minvalue;
			params.maxvalue = update.maxvalue;
		}
	};

	this.update = function(val) {
		last_update = unix_ts();//new Date();
		this.propagateUpdate();
		this.device.update();
	};

	Object.defineProperties(this, {
		'eid': {
			enumerable: true,
			get: function() { return eid; }
		},
		'id': {
			enumerable: true,
			get: function() { return endpoint_id(device.ip, eid); }
		},
		'ip': {
			enumerable: true,
			get: function() { return device.ip; }
		},
		'device': {
			get: function() { return device; }
		},
		'name': {
			enumerable: true,
			get: function() { return device.name; }
		},
		'last_update': {
			enumerable: true,
			get: function() { return last_update; }
		},
		'last_value': {
			get: function() { return last_value; },
			set: function(val) {
				last_value = val;
				//Takes also care of calling propagateUpdate
				this.update();
				emitter.emit('endpoint_new_value', this);
			},
		},
		'age': {
			get: function() {
				return unix_ts() - this.last_update;
			}
		}
	});

	var endpointKeys = ["type", "description", "function"];
	var functionKeys = {
		'sensor': {
			'minvalue': {
				enumerable: true,				
				get: function() { return params.minvalue; },
				set: function(val) {
					params.minvalue  = val;
					this.propagateUpdate();
				}
			},
			'maxvalue': {
				enumerable: true,				
				get: function() { return params.maxvalue; },
				set: function(val) {
					params.maxvalue  = val;
					this.propagateUpdate();
				}
			},
			'unit': {
				enumerable: true,
				get: function() { return params.unit; }
			},
		},
	};

	var addEndpointKeys = function(target) {
		for(var index in endpointKeys) {
			Object.defineProperty(target, endpointKeys[index], {
				enumerable: true,
				writable: false,
				value: params[endpointKeys[index]]
			});
		}
	};

	var addFunctionKeys = function(target) {
		if(functionKeys[params.function] !== undefined) {
			Object.defineProperties(target, functionKeys[params.function]);
		}
	};

	this.toJSON = function() {
		var json = {};
		for (var key in this) {
			json[key] = this[key];
		}

		return json;
	};

	var setMinMaxValues = function() {
		if(params.function == "sensor" && 
			(params.minvalue === undefined || params.maxvalue === undefined)) {
			//FIXME: Load min max values from config file
			switch (eid) {
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
	};

	if (arguments.length == 1) {
		var obj = device;

		device = obj.device;
		emitter = obj.emitter;

		eid = obj.ep.eid;
		last_update = obj.ep.last_update;
		last_value = obj.ep.last_value;
		params = obj.ep;
	} else {
		last_update = unix_ts();
	}

	if (eid === undefined)
		throw "Required parameter: eid";
	eid = parseInt(eid);
	if (isNaN(eid))
		throw "Invalid: eid. Expected: int";

	
	if(typeof params.type != "number") {
		console.trace("Field type is:" + params.type);
		throw "Field type is supposed to be a number.";
	}

	setMinMaxValues();
	addEndpointKeys(this);
	addFunctionKeys(this);

	last_update = last_update || unix_ts();
	last_value = last_value || 0;
};

var View = function(name, devices, emitter, id) {

	this.propagateUpdate = function() {
		var update = {};
		update.views = {};
		update.views[id] = this.toJSON();
		emitter.propagateUpdate(update);
	};

	this.applyUpdate = function(update) {
		if(update.name === undefined || update.devices === undefined) {
			throw 'Invalid format for view update';
		}

		name = update.name;
		devices = update.devices;
	};

	this.toJSON = function() {
		var json = {
			'id' : id,
			'name' : name,
			'devices' : devices
		};

		return json;
	};

	Object.defineProperties(this, {
		"id": {
			enumerable: true,
			get: function() {
				return id; 
			}
		},
		"name": {
			enumerable: true,
			get: function() { 
				return name;
			},
			set: function(newName) {
				name = newName;
				this.propagateUpdate();
			}
		},
		"devices" : {
			enumerable: true,
			get: function() {
				return devices;
			},
			set: function(newDevices) {
				devices = newDevices;
				this.propagateUpdate();				
			} 
		}
	});

	if(name === undefined) {
		throw "Required parameter: name";
	}

	devices = devices || {};
	
	if(emitter === undefined) {
		throw "Required parameter: emitter";
	}

	id = id || uuid.v4();
};

var DeviceTree = function(file) {
	var devices = {};
	var views = {};

	this.applyUpdate = function(update) {
		if(update.devices !== undefined) {
			for(var ip in update.devices) {
				if(devices[ip] !== undefined) {
					devices[ip].applyUpdate(update.devices[ip]);
				}
				else {
					var newDevice = update.devices[ip];
					devices[ip] = new Device(ip, 
											newDevice.name, 
											this, 
											newDevice.last_update);
				}
			}
		}
	
		if(update.views !== undefined) {
			for(var id in update.views) {
				if(views[id] !== undefined) {
					views[id].applyUpdate(update.views[id]);
				} 
				else {
					var newView = update.views[id]; 
					views[id] = new View(newView.name, newView.devices, this, id);
				}
			}
		}
	};

	this.applyDeletion = function(deletion) {
		if(deletion.views !== undefined) {
			for(var viewId in deletion.views) {
				if(isEmptyObject(deletion.views[viewId])) {
					delete views[viewId];
				}
			}
		}

		if(deletion.devices !== undefined) {
			for(var deviceIp in deletion.devices) {
				if(isEmptyObject(deletion.devices[deviceIp])) {
					delete devices[deviceIp];
				}
				else {
					devices[deviceIp].applyDeletion(deletion.devices[deviceIp]);
				}
			}
		}
	};

	this.propagateUpdate = function(update) {
		this.emit('update', update);
	};

	this.propagateDeletion = function(deletion) {
		this.emit('delete', deletion);
	};

	this.propagateViewDeletion = function(viewId) {
		var deletion = {};
		deletion.views = {};
		deletion.views[viewId] = {};
		this.propagateDeletion(deletion);
	};

	this.propagateDeviceDeletion = function(ip) {
		var deletion = {};
		deletion.devices = {};
		deletion.devices[ip] = {};
		this.propagateDeletion(deletion);
	};

	this.addDevice = function(ip, name) {
		if(devices[ip] !== undefined) {
			throw "Device does already exist";
		}

		var device = new Device(ip, name, this);
		devices[ip] = device;

		device.propagateUpdate();

		return device;
	};

	this.removeDevice = function(ip) {
		if(devices[ip] === undefined) {
			throw "Device not found";
		}

		delete devices[ip];
		this.propagateDeviceDeletion(ip);
	};

	this.add_endpoint = function(ip, eid, params) {
		var device = devices[ip] || this.addDevice(ip, params.name); 
		var endpoint = device.endpoints[eid] || device.addEndpoint(eid, params); 

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
		//TODO: Refactor all code to use removeDevice instead
		this.removeDevice(ip);
		this.emit('device_remove', devices[ip]);
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

	this.addView = function(name, devices) {
		var view = new View(name, devices, this);
		views[view.id] = view;

		view.propagateUpdate(view);

		return view;
	};

	this.removeView = function(id) {
		if(views[id] !== undefined) {
			delete views[id];
			this.propagateViewDeletion(id);
		}
		else {
			throw "No such view";
		}
	};

	Object.defineProperties(this, {
		devices: {
			value: devices
		},
		views: {
			get: function() {
				return views;
			}
		}
	});

	this.toJSON = function() {
		var json = {
			devices: devices,
			views: views
		};
		return json;
	};

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

	if (file) {
		var data = fs.readFileSync(file);
		var json = JSON.parse(data);

		for (var key in json.devices) {
			devices[key] = new Device({
				emitter: this,
				dev: json.devices[key]
			});
		}
		
		for(var id in json.views) {
			var view = json.views[id];
			views[id] = new View(view.name, view.devices, this, id);
		}
	}

};

util.inherits(DeviceTree, EventEmitter);
module.exports = DeviceTree;
