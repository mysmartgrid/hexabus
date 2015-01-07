'use strict';

/*
 * The code in this file can be run inside nodejs
 * as well as inside the browser.
 *
 * Therefore special wrapper pattern is requried.
 * See: http://caolanmcmahon.com/posts/writing_for_node_and_the_browser/
 */

var moduleWrapper = function(globalScope) {

	var inherits, EventEmitter, uuid;
	var isServerDeviceTree;

	/*
	 * Check the type of require because if we have a require function, 
	 * we can be quite certain that this is nodejs
	 */
	 if(typeof require == 'function') {
		//Nodejs specific code
		inherits = require('util').inherits;
		EventEmitter = require('events').EventEmitter;
		uuid = require('node-uuid');

		isServerDeviceTree = true;
	}
	else {
		// Browser specific code
		inherits = globalScope.inherits;
		EventEmitter = globalScope.EventEmitter;
		uuid = globalScope.uuid;

		isServerDeviceTree = false;
	}

	/*
	 * Decorator that marks a function unaviable on the browserside
	 * Example: this.doServerStuff = onServer(function(foo, barz) { });
	 */
	var onServer = function(fun) {
		if(isServerDeviceTree) {
			return fun;
		}
		return undefined;
	};

	/*
	 *  Helper function to get seconds since the epoch (instead of milliseconds)
	 */
	var unix_ts = function() {
		return Math.round(new Date() / 1000);
	};

	/*
	 * Check if the object is {}
	 */ 
	var isEmptyObject = function(obj) {
		return (Object.keys(obj).length === 0);
	};

	/*
	 * Constructor for objects representing a hexabus device
	 *
	 * This Constructor can be used in two ways
	 *	1. new Device(ip, name, sm_name, emitter, rest)
	 *  2. new Device(jsonRepresentation)
	 *
	 * Members:
	 * ip 					ipv6 address of the device
	 * name 				name of the device (as provided by hexinfo)
	 * sm_name				unique device name (as provided by hexinfo)
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
	var Device = function(ip, name, sm_name, emitter, last_update) {
		var endpoints = {};

		/*
		 * This function propagates the a update of this device on level up in the devicetree.
		 * It can be used in two ways:
		 * 1. propagateUpdate()
		 * In this 
		 *
		 */
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

		this.addEndpoint = onServer(function(eid, params) {
			if(endpoints[eid] !== undefined) {
				throw "Endpoint already exists";
			}

			var endpoint = new Endpoint(this, eid, params, emitter);
			endpoints[eid] = endpoint;

			endpoint.propagateCreation(params);

			return endpoint;
		});

		this.removeEndpoint = onServer(function(eid) {
			if(endpoints[eid] === undefined) {
				throw "Endpoint not found";
			}

			delete endpoints[eid];
			this.propagteEnpointDeletion(eid);
		});

		Object.defineProperties(this, {
			'ip': {
				get: function() { return ip; },
				enumerable: true
			},
			'name': {
				enumerable: true,
				get: function() { return name; },
				set: onServer(function(val) {
					name = val;
					//takes also care of calling propagateUpdate
					this.update();
					emitter.emit('device_renamed', this);
				})
			},
			'sm_name': {
				enumerable: true,
				get: function() { return sm_name; }
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
				'sm_name' : sm_name,
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
			sm_name = obj.dev.sm_name;
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
		if (sm_name === undefined)
			throw "Required parameter: sm_name";

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
				enumerable: true,
				get: function() { return last_value; },
				set: onServer(function(val) {
					last_value = val;
					//Takes also care of calling propagateUpdate
					this.update();
					emitter.emit('endpoint_new_value', this);
				}),
			},
			'age': {
				get: function() {
					return unix_ts() - this.last_update;
				}
			},
			'associated': {
				get: function() {
					if(eid == 1) {
						return { 2 : this.device.endpoints[2]};
					}
					else if(eid == 2) {
						return {1: this.device.endpoints[1]};
					}
				}
			},
			'timeout': {
				get: function() {
					return this.age > 600;
				}
			}
		});

		var endpointKeys = ["sm_name", "type", "description", "function"];
		var functionKeys = {
			'sensor': {
				'minvalue': {
					enumerable: true,
					get: function() { return params.minvalue; },
					set: onServer(function(val) {
						params.minvalue  = val;
						this.propagateUpdate();
					})
				},
				'maxvalue': {
					enumerable: true,
					get: function() { return params.maxvalue; },
					set: onServer(function(val) {
						params.maxvalue  = val;
						this.propagateUpdate();
					})
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

				case 4: //internal button
					params.minvalue = 0;
					params.maxvalue = 1;
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
		if (isNaN(eid)) {
			throw "Invalid: eid. Expected: int";
		}

		if(typeof params.type != "string") {
			console.trace("Field type is:" + params.type);
			throw "Field type is supposed to be a string.";
		}

		setMinMaxValues();
		addEndpointKeys(this);
		addFunctionKeys(this);

		last_update = last_update || unix_ts();
		last_value = last_value || 0;
	};


	var View = function(name, endpoints, emitter, id) {

		this.propagateUpdate = function() {
			var update = {};
			update.views = {};
			update.views[id] = this.toJSON();
			emitter.propagateUpdate(update);
		};

		this.applyUpdate = function(update) {
			if(update.name === undefined || update.endpoints === undefined) {
				throw 'Invalid format for view update';
			}

			name = update.name;
			endpoints = update.endpoints;
		};

		this.toJSON = function() {
			var json = {
				'id' : id,
				'name' : name,
				'endpoints' : endpoints
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
			"endpoints" : {
				enumerable: true,
				get: function() {
					return endpoints;
				},
				set: function(newEndpoints) {
					endpoints = newEndpoints;
					this.propagateUpdate();
				}
			}
		});

		if(name === undefined) {
			throw "Required parameter: name";
		}

		endpoints = endpoints || [];

		if(emitter === undefined) {
			throw "Required parameter: emitter";
		}

		id = id || uuid.v4();
	};

	var DeviceTree = function(jsonTree) {
		var devices = {};
		var views = {};

		this.applyUpdate = function(update) {
			if(update.devices !== undefined) {
				if(isServerDeviceTree) {
					throw 'Can not update devices subtree on server side.';
				}

				for(var ip in update.devices) {
					if(devices[ip] !== undefined) {
						devices[ip].applyUpdate(update.devices[ip]);
					}
					else {
						var newDevice = update.devices[ip];
						devices[ip] = new Device(ip,
												newDevice.name,
												newDevice.sm_name,
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
						views[id] = new View(newView.name, newView.endpoints, this, id);
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
						if(isServerDeviceTree) {
							throw 'Can not delete inside a device on server side';
						}
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

		this.addDevice = onServer(function(ip, name, sm_name) {
			if(devices[ip] !== undefined) {
				throw "Device does already exist";
			}

			var device = new Device(ip, name, sm_name, this);
			devices[ip] = device;

			device.propagateUpdate();

			return device;
		});

		this.removeDevice = function(ip) {
			if(devices[ip] === undefined) {
				throw "Device not found";
			}

			delete devices[ip];
			this.propagateDeviceDeletion(ip);
		};

		this.add_endpoint = onServer(function(ip, eid, params) {
			var device, endpoint;

			if (devices[ip] !== undefined) {
				device = devices[ip];
			}
			else {
				device = this.addDevice(ip, params.name);
				this.emit('device_new', device);
			}


			if (device.endpoints[eid] !== undefined) {
				throw 'Endpoint exists';
			}
			else {
				endpoint = device.addEndpoint(eid, params);
				this.emit('endpoint_new', endpoint);
			}

			return endpoint;
		});

		this.forEach = function(cb) {
			for (var key in devices) {
				if (devices.hasOwnProperty(key)) {
					cb(devices[key]);
				}
			}
		};

		this.remove = onServer(function(ip) {
			//TODO: Refactor all code to use removeDevice instead
			this.removeDevice(ip);
			this.emit('device_remove', devices[ip]);
		});

		this.endpoint_by_id = function(id) {
			var parts = id.split('.');
			var dev = devices[parts[0]];
			if (!dev) {
				return undefined;
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

		this.reset = onServer(function() {
			for(var ip in devices) {
				this.removeDevice(ip);
			}
		
			for(var id in view) {
				this.removeView(id);
			}
		});

		Object.defineProperties(this, {
			devices: {
				get: function() {
					return devices;
				}
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
		
		if (jsonTree !== undefined) {
			for (var key in jsonTree.devices) {
				devices[key] = new Device({
					emitter: this,
					dev: jsonTree.devices[key]
				});
			}

			for(var id in jsonTree.views) {
				var view = jsonTree.views[id];
				views[id] = new View(view.name, view.endpoints, this, id);
			}
		}

	};


	inherits(DeviceTree, EventEmitter);

	if(typeof module !== 'undefined' && typeof module.exports !== 'undefined') {
		module.exports = DeviceTree;
	}
	else {
		globalScope.DeviceTree = DeviceTree;
	}

};

if(typeof module !== 'undefined' && typeof module.exports !== 'undefined') {
	moduleWrapper();
}
else {
	/* jshint -W040 */
	moduleWrapper(this);
	/* jshint +W040 */
	console.log('Sucessfully initialized the devicetree module');
}
