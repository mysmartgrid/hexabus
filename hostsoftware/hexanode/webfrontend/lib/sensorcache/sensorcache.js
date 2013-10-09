const BUFFER_SIZE=4;
var CBuffer=require("CBuffer");
var util = require('util');
var EventEmitter = require('events').EventEmitter;


var SensorCache = function() {
	var values = {};

	var unix_ts = function() {
		return Math.round((+new Date()) / 1000);
	}

	this.add_value = function(sensor, value) {
		values[sensor.id] = {
			ts: unix_ts(),
			value: value
		};
		this.emit('sensor_update', { sensor: sensor.id, device: sensor.ip, value: values[sensor.id] });
	}

	this.get_current_value = function(sensor) {
		return values[sensor.id];
	}

	this.enumerate_current_values = function(cb) {
		cb = cb || function() {};
		for (var key in values) {
			cb({ sensor: key, value: values[key] });
		}
	};

};

util.inherits(SensorCache, EventEmitter);
module.exports = SensorCache;
