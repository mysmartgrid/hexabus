var application_root = __dirname
  , path=require('path')
  , express=require('express')
  , connect = require('connect')
  , app = module.exports = express()
  , server=require("http").createServer(app)
  , io = require('socket.io').listen(server)
	, Cache = require("./lib/sensorcache")
	, SensorRegistry = require("./lib/sensorregistry")
	, hexabus = require("./lib/hexabus")
	, fs = require("fs")
  , nconf=require('nconf');

nconf.env().argv();
// Setting default values
nconf.defaults({
  'port': '3000',
  'server': '10.23.1.250',
	'config': '.'
});

server.listen(nconf.get('port'));

// drop root if we ever had it, our arguments say so.
// this includes all supplemental groups, of course
if (nconf.get('uid')) {
	var uid = nconf.get('uid');
	uid = parseInt(uid) || uid;
	var gid = nconf.get('gid') || uid;
	gid = parseInt(gid) || gid;

	process.setgid(gid);
	process.setgroups([gid]);
	process.setuid(uid);
}

var configDir = nconf.get('config');
var sensors_file = configDir + '/sensors.json';
var sensor_registry;
try {
	sensor_registry = new SensorRegistry(fs.existsSync(sensors_file) ? sensors_file : undefined);
} catch (e) {
	sensor_registry = new SensorRegistry();
}
var sensor_cache = new Cache();
console.log("Using configuration: ");
console.log(" - server: " + nconf.get('server'));
console.log(" - port: " + nconf.get('port'));
console.log(" - config dir: " + nconf.get('config'));


// see http://stackoverflow.com/questions/4600952/node-js-ejs-example
// for EJS
app.configure(function () {
  app.set('views', __dirname + '/views');
  app.set('view engine', 'ejs');
//  app.use(connect.logger('dev'));
  app.use(express.bodyParser());
  app.use(express.methodOverride());
  app.use(app.router);
  app.use(express.static(path.join(application_root, "public")));
  app.use(express.errorHandler({ dumpExceptions: true, showStack: true }));
});


app.get('/api', function(req, res) {
  res.send("API is running.");
});

app.get('/api/sensor', function(req, res) {
	res.send(JSON.stringify(sensor_registry.get_sensors()));
});

app.get('/api/sensor/:ip/:eid/latest', function(req, res) {
	var sensor = sensor_registry.get_sensor(req.params.ip, req.params.eid);
	if (!sensor) {
		res.send("Sensor not found", 404);
	} else if (sensor_cache.get_current_value(sensor) == undefined) {
		res.send("No value yet", 404);
	} else {
		res.send(JSON.stringify(sensor_cache.get_current_value(sensor.id)));
	}
});

app.get('/api/sensor/:ip/:eid/info', function(req, res) {
	var sensor = sensor_registry.get_sensor(req.params.ip, req.params.eid);
	if (sensor) {
		res.send(JSON.stringify(sensor));
	} else {
		res.send("Sensor not found", 404);
	}
});

app.put('/api/sensor/:ip/:eid', function(req, res) {
	if (sensor_registry.get_sensor(req.params.ip, req.params.eid)) {
		res.send("Sensor exists", 200);
	} else {
		try {
			var sensor = sensor_registry.add_sensor(req.params.ip, req.params.eid, req.body);
			sensor_cache.add_value(sensor, parseFloat(req.body.value));
			sensor_registry.save(sensors_file);
			res.send("Sensor added", 200);
		} catch(err) {
			res.send(err, 400);
		}
	}
});

app.post('/api/sensor/:ip/:eid', function(req, res) {
	var sensor = sensor_registry.get_sensor(req.params.ip, req.params.eid);
	var value = parseFloat(req.body.value);
	if (!sensor) {
		res.send("Not found", 404);
	} else if (isNaN(value)) {
		res.send("Bad value", 400);
	} else {
		sensor_cache.add_value(sensor, value);
		res.send("Value added", 200);
	}
});

app.post('/api/device/rename/:ip', function(req, res) {
	try {
		if (!req.body.name) {
			res.send("No name given", 400);
		}
		hexabus.rename_device(req.params.ip, req.body.name, function() {
			res.send("Success");
		});
	} catch (err) {
		console.log(err);
		res.send(err, 500);
	}
});

app.get('/', function(req, res) {
  res.render('index.ejs');
});

io.sockets.on('connection', function (socket) {
  console.log("Registering new client.");
	sensor_cache.on('sensor_update', function(update) {
		socket.volatile.emit('sensor_update', update);
	});
	sensor_registry.on('sensor_new', function(sensor) {
		socket.volatile.emit('sensor_new', sensor);
	});
	socket.on('sensor_request_metadata', function(id) {
		var sensor = sensor_registry.get_sensor_by_id(id);
		if (sensor) {
			socket.volatile.emit('sensor_metadata', sensor);
			var value = { sensor: id, value: sensor_cache.get_current_value(sensor) };
			if (value.value != undefined) {
				socket.volatile.emit('sensor_update', value);
			}
		}
	});
  // a client can also initiate a data update.
  socket.on('sensor_refresh_data', function() {
  //  console.log("Refresh data event.");
		sensor_cache.enumerate_current_values(function(update) {
			socket.volatile.emit('sensor_update', udpate);
		});
  });
	socket.on('device_rename', function(msg) {
		var sensors = sensor_registry.get_sensors(function(sensor) {
			return sensor.ip == msg.device;
		});
		if (sensors.length > 0) {
			try {
				hexabus.rename_device(msg.device, msg.name, function(err) {
					if (err) {
						console.log(err);
						return;
					}
					for (var key in sensors) {
						sensors[key].name = msg.name;
						socket.broadcast.emit('sensor_metadata', sensors[key]);
						socket.emit('sensor_metadata', sensors[key]);
						var value = sensor_cache.get_current_value(sensors[key]);
						if (value) {
							socket.broadcast.emit('sensor_update', value);
							socket.emit('sensor_update', value);
						}
					}
				});
			} catch (err) {
				console.log(err);
				socket.emit('device_rename_error', { device: msg.device, error: err });
			}
		} else {
			socket.emit('device_rename_error', { device: msg.device, error: "No such device" });
		}
	});
	socket.on('sensor_change_metadata', function(msg) {
		var sensor = sensor_registry.get_sensor_by_id(msg.id);
		if (sensor) {
			var keys = ["minvalue", "maxvalue"];
			for (var i in keys) {
				var key = keys[i];
				sensor[key] = msg.data[key] || sensor[key];
			}
			sensor_registry.save(sensors_file);
			socket.broadcast.emit('sensor_metadata', sensor);
			socket.emit('sensor_metadata', sensor);
			var value = { sensor: sensor.id, value: sensor_cache.get_current_value(sensor) };
			if (value.value) {
				socket.broadcast.emit('sensor_update', value);
				socket.emit('sensor_update', value);
			}
		}
	});

	socket.emit('clear_state');

	var list = sensor_registry.get_sensors();
	for (var sensor in list) {
		socket.emit('sensor_metadata', list[sensor]);
	}
	sensor_cache.enumerate_current_values(function(value) {
		socket.emit('sensor_update', value);
	});
});

