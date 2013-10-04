var util = require('util');
var EventEmitter = require('events').EventEmitter;
var fs = require('fs');

var SensorRegistry = function(file) {
	var sensors = {};

	if (file) {
		fs.readFile(file, function(err, data) {
			if (err)
				throw err;
			sensors = JSON.parse(data);
		});
	}

	var unix_ts = function() {
		return Math.round((+new Date()) / 1000);
	}

	var sensor_id = function(ip, eid) {
		if (ip == undefined)
			throw "ip required";
		if (eid == undefined)
			throw "eid required";

		return ip + "." + eid;
	};

	this.add_sensor = function(ip, eid, params) {

		var sensor = {
			ip: ip,
			eid: eid,

			id: sensor_id(ip, eid),
			last_update: unix_ts()
		};

		var keys = ["name", "minvalue", "maxvalue", "type", "unit"];
		for (var key in keys) {
			if (!(keys[key] in params)) {
				throw keys[key] + " required";
			}
			sensor[keys[key]] = params[keys[key]];
		}

		sensors[sensor.id] = sensor;
		this.emit('sensor_new', sensor);
		return sensor;
	};

	this.age_of = function(ip, eid) {
		var sensor = this.get_sensor(ip, eid);
		if (!sensor) {
			throw "No such sensor";
		}
		return unix_ts() - sensor.last_update;
	};

	this.updated = function(ip, eid) {
		var sensor = this.get_sensor(ip, eid);
		if (!sensor) {
			throw "No such sensor";
		}
		sensor.last_updated = unix_ts();
	};

	this.get_sensors = function(fn) {
		var result = [];
		fn = fn || function() { return true; };
		for (var id in sensors) {
			if (fn(sensors[id])) {
				result.push(sensors[id]);
			}
		}
		return result;
	};

	this.get_sensor = function(ip, eid) {
		return sensors[sensor_id(ip, eid)];
	};

	this.get_sensor_by_id = function(id) {
		return sensors[id];
	};

	this.save = function(file, cb) {
		cb = cb || function() {};

		fs.writeFile(file, JSON.stringify(sensors), function(err) {
			if (err)
				throw err;
		});
	};
};

util.inherits(SensorRegistry, EventEmitter);
module.exports = SensorRegistry;
