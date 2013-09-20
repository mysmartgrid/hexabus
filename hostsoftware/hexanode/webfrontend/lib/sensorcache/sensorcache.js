const BUFFER_SIZE=4;
var CBuffer=require("CBuffer");
var util = require('util');
var EventEmitter = require('events').EventEmitter;


var SensorCache = function() {
	var sensors = {};

	var unix_ts = function() {
		return Math.round((+new Date()) / 1000);
	}

	var render_sensor_lastvalue = function(sensor) {
		return {
			id: sensor.id,
			value: sensor.values.last()
		};
	}

	var render_sensor_info = function(sensor) {
		return {
			id: sensor.id,
			name: sensor.name,
			displaytype: sensor.displaytype,
			minvalue: sensor.minvalue,
			maxvalue: sensor.maxvalue,
			values: sensor.values.toArray()
		};
	}

	this.get_sensor_info_list = function() {
		var result = [];
		for (var id in sensors) {
			result.push(render_sensor_info(sensors[id]));
		}
		return result;
	}

	this.add_sensor = function(id, params) {
		console.log("Attempting to create sensor: " + JSON.stringify(params));

		if (id == undefined)
			throw "id required";
		if (params.name == undefined)
			throw "name required";
		if (params.minvalue == undefined)
			throw "minvalue required";
		if (params.maxvalue == undefined)
			throw "maxvalue required";
		if (params.displaytype == undefined)
			throw "displaytype required";

		var sensor = {
			id: id,
			name: params.name,
			minvalue: params.minvalue,
			maxvalue: params.maxvalue,
			displaytype: params.displaytype,

			values: new CBuffer(BUFFER_SIZE)
		};

		if (params.value != undefined) {
			var entry = {};
			entry[unix_ts()] = params.value;
			sensor.values.push(entry);
		}

		sensors[sensor.id] = sensor;
		this.emit('sensor_new', render_sensor_info(sensor));
		return sensor;
	}

	this.get_sensor_info = function(id) {
		if (id in sensors) {
			return render_sensor_info(sensors[id]);
		} else {
			return null;
		}
	}

	this.enumerate_current_values = function(callback) {
		for (id in sensors) {
			callback(render_sensor_lastvalue(sensors[id]));
		}
	}

	this.add_value = function(id, value) {
		if (!id in sensors) {
			throw "Sensor not found";
		}
		if (!value) {
			throw "Value required";
		}
		var sensor = sensors[id];
		var entry = {};
		entry[unix_ts()] = value;
		sensor.values.push(entry);
		this.emit('sensor_update', render_sensor_lastvalue(sensor));
	}

	this.get_last_value = function(id) {
		if (id in sensors) {
			return sensors[id].values.last();
		} else {
			return null;
		}
	}

};

util.inherits(SensorCache, EventEmitter);
module.exports = SensorCache;
