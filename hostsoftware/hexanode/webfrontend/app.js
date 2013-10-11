var application_root = __dirname
  , path=require('path')
  , express=require('express')
  , connect = require('connect')
  , app = module.exports = express()
  , server=require("http").createServer(app)
  , io = require('socket.io').listen(server)
	, DeviceTree = require("./lib/devicetree")
	, Hexabus = require("./lib/hexabus")
	, hexabus = new Hexabus()
	, Wizard = require("./lib/wizard")
	, fs = require("fs")
  , nconf=require('nconf');

nconf.env().argv();
// Setting default values
nconf.defaults({
  'port': '3000',
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
var devicetree_file = configDir + '/devicetree.json';

var devicetree;

function open_config() {
	try {
		devicetree = new DeviceTree(fs.existsSync(devicetree_file) ? devicetree_file : undefined);
	} catch (e) {
		devicetree = new DeviceTree();
	}
}
open_config();

var save_devicetree = function(cb) {
	devicetree.save(devicetree_file, cb || Object);
};

setInterval(save_devicetree, 10 * 60 * 1000);
process.on('SIGINT', function() {
	save_devicetree(process.exit);
});
process.on('SIGTERM', function() {
	save_devicetree(process.exit);
});

console.log("Using configuration: ");
console.log(" - port: " + nconf.get('port'));
console.log(" - config dir: " + nconf.get('config'));

if (nconf.get('socket-debug') !== undefined) {
	io.set('log level', nconf.get('socket-debug'));
} else {
	io.set('log level', 1);
}


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

//app.get('/api/sensor', function(req, res) {
//	res.send(JSON.stringify(sensor_registry.get_sensors()));
//});
//
//app.get('/api/sensor/:ip/:eid/latest', function(req, res) {
//	var sensor = sensor_registry.get_sensor(req.params.ip, req.params.eid);
//	if (!sensor) {
//		res.send("Sensor not found", 404);
//	} else if (sensor_cache.get_current_value(sensor) == undefined) {
//		res.send("No value yet", 404);
//	} else {
//		res.send(JSON.stringify(sensor_cache.get_current_value(sensor.id)));
//	}
//});
//
//app.get('/api/sensor/:ip/:eid/info', function(req, res) {
//	var sensor = sensor_registry.get_sensor(req.params.ip, req.params.eid);
//	if (sensor) {
//		res.send(JSON.stringify(sensor));
//	} else {
//		res.send("Sensor not found", 404);
//	}
//});

app.put('/api/sensor/:ip/:eid', function(req, res) {
	var device = devicetree.devices[req.params.ip];
	if (device && device.endpoints[req.params.eid]) {
		res.send("Sensor exists", 200);
	} else {
		try {
			var endpoint = devicetree.add_endpoint(req.params.ip, req.params.eid, req.body);
			endpoint.last_value = {
				unix_ts: Math.round(Date.now() / 1000),
				value: parseFloat(req.body.value)
			};
			res.send("Sensor added", 200);
		} catch(err) {
			console.log(err);
			console.log(err.stack);
			res.send(JSON.stringify(err), 400);
		}
	}
});

app.post('/api/sensor/:ip/:eid', function(req, res) {
	var device = devicetree.devices[req.params.ip];
	if (!device || !device.endpoints[req.params.eid]) {
		res.send("Not found", 404);
	} else {
		var value = parseFloat(req.body.value);
		if (isNaN(value)) {
			res.send("Bad value", 400);
		} else {
			device.endpoints[req.params.eid].last_value = {
				unix_ts: Math.round(Date.now() / 1000),
				value: value
			};
			res.send("Value added");
		}
	}
});

app.post('/api/device/rename/:ip', function(req, res) {
	if (!req.body.name) {
		res.send("No name given", 400);
	} else {
		hexabus.rename_device(req.params.ip, req.body.name, function(err) {
			if (err) {
				res.send(JSON.stringify(err), 500);
			} else {
				res.send("Success");
				var device = devicetree.devices[req.params.ip];
				if (device) {
					device.name = req.body.name;
				}
			}
		});
	}
});

app.get('/', function(req, res) {
	fs.exists('/etc/radvd.conf', function(exists) {
		if (exists) {
			res.render('index.ejs', { active_tabpage: 'dashboard' });
		} else {
			res.redirect('/wizard/new');
		}
	});
});
app.get('/wizard', function(req, res) {
	fs.exists('/etc/radvd.conf', function(exists) {
		if (exists) {
			res.redirect('/wizard/current');
		} else {
			res.redirect('/wizard/new');
		}
	});
});
app.get('/wizard/current', function(req, res) {
	var config = hexabus.read_current_config();
	hexabus.get_heartbeat_state(function(err, state) {
		config.active_tabpage = 'configuration';

		if (err) {
			config.heartbeat_ok = false;
			config.heartbeat_messages = [err];
		} else {
			config.heartbeat_ok = state.code == 0;
			config.heartbeat_code = state.code;
			config.heartbeat_messages = state.messages;
			config.heartbeat_state = state;
		}

		res.render('wizard/current.ejs', config);
	});
});
app.post('/wizard/reset', function(req, res) {
	try {
		fs.unlinkSync(sensors_file);
	} catch (e) {
	}
	open_config();

	var wizard = new Wizard();
	wizard.deconfigure_network(function(err) {
		if (err) {
			res.send(JSON.stringify({ step: "deconfigure_network", error: err }), 500);
			return;
		}
		wizard.unregisterMSG(function(err) {
			if (err) {
				res.send(JSON.stringify({ step: "unregisterMSG", error: err }), 500);
				return;
			}
			res.redirect('/wizard/resetdone');
		});
	});
});
app.get('/wizard/resetdone', function(req, res) {
	res.render('wizard/resetdone.ejs', { active_tabpage: 'configuration' });
	var wizard = new Wizard();
	wizard.complete_reset();
});
app.get('/wizard/devices', function(req, res) {
	var devices = {};

	devicetree.forEach(function(device) {
		var entry = devices[device.ip] = { name: device.name, ip: device.ip, eids: [] };

		device.forEachEndpoint(entry.eids.push);
		entry.eids.sort(function(a, b) {
			return b.eid - a.eid;
		});
	});

	res.render('wizard/devices.ejs', { active_tabpage: 'configuration', devices: devices });
});
app.get('/wizard/:step', function(req, res) {
	res.render('wizard/' + req.params.step  + '.ejs', { active_tabpage: 'configuration' });
});

var ep_is_old = function(ep) {
	return ep.age >= 60 * 60 * 1000;
};

setInterval(function() {
	devicetree.forEach(function(device) {
		device.forEachEndpoint(function(ep) {
			if (ep.function == "sensor" && ep_is_old(ep)) {
				devicetree.emit('sensor_timeout', { sensor: ep });
			}
		});
	});
}, 1000);

io.sockets.on('connection', function (socket) {
  console.log("Registering new client.");

	var broadcast_sensor = function(sensor) {
		socket.broadcast.emit('sensor_metadata', sensor);
		socket.emit('sensor_metadata', sensor);
	};
	var send_sensor_update = function(ep) {
		socket.emit('sensor_update', { sensor: ep.id, device: ep.device.ip, value: ep.last_value });
	};

	devicetree.on('endpoint_new_value', function(ep) {
		if (ep.function == "sensor") {
			send_sensor_update(ep);
		}
	});
	devicetree.on('endpoint_new', function(ep) {
		if (ep.function == "sensor") {
			socket.emit('sensor_new', ep);
		}
	});
	devicetree.on('device_renamed', function(dev) {
		dev.forEachEndpoint(broadcast_sensor);
	});
	devicetree.on('sensor_timeout', function(msg) {
		socket.emit('sensor_timeout', msg);
	});

	socket.on('sensor_request_metadata', function(id) {
		var sensor = devicetree.endpoint_by_id(id);
		if (sensor) {
			socket.emit('sensor_metadata', sensor);
			if (sensor.last_value != undefined) {
				send_sensor_update(sensor);
			}
		}
	});
	socket.on('device_rename', function(msg) {
		var device = devicetree.devices[msg.device];
		if (!device) {
			socket.emit('device_rename_error', { device: msg.device, error: 'Device not found' });
			return;
		}
		if (!msg.name) {
			socket.emit('device_rename_error', { device: msg.device, error: 'No name given' });
			return;
		}

		hexabus.rename_device(msg.device, msg.name, function(err) {
			if (err) {
				socket.emit('device_rename_error', { device: msg.device, error: err });
				return;
			}
			device.name = msg.name;
		});
	});
	socket.on('sensor_change_metadata', function(msg) {
		var sensor = devicetree.endpoint_by_id(msg.id);
		if (sensor) {
			["minvalue", "maxvalue"].forEach(function(key) {
				sensor[key] = (msg.data[key] != undefined) ? msg.data[key] : sensor[key];
			});
			save_devicetree();
			broadcast_sensor(sensor);
		}
	});

	socket.on('wizard_configure', function() {
		var wizard = new Wizard();

		wizard.configure_network(function(progress) {
			socket.emit('wizard_configure_step', progress);
		});
	});

	socket.on('wizard_register', function() {
		var wizard = new Wizard();

		wizard.registerMSG(function(progress) {
			socket.emit('wizard_register_step', progress);
		});
	});

	socket.emit('clear_state');

	devicetree.forEach(function(device) {
		device.forEachEndpoint(function(ep) {
			if (ep.function == "sensor" && !ep_is_old(ep)) {
				socket.emit('sensor_metadata', ep);
				if (ep.last_value) {
					send_sensor_update(ep);
				}
			}
		});
	});
});

