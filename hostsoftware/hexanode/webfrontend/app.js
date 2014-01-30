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

nconf.env().argv().file({file: 'config.json'});
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
		console.log(e);
		devicetree = new DeviceTree();
	}
}
open_config();

var save_devicetree = function(cb) {
	devicetree.save(devicetree_file, cb || Object);
};

var add_endpoint = function(ip, eid, params) {
	// FIXME: ugly hack for min/maxvalues, move this to $somewhere_useful
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
	return devicetree.add_endpoint(ip, eid, params);
};

var ignore_endpoint = function(ip, eid) {
	return eid == 7 || eid == 8;
};

var enumerate_network = function(cb) {
	cb = cb || Object;
	hexabus.enumerate_network(function(dev) {
		if (dev.done) {
			cb();
		} else if (dev.error) {
			console.log(dev.error);
		} else {
			dev = dev.device;
			for (var key in dev.endpoints) {
				var ep = dev.endpoints[key];
				ep.name = ep.name || dev.name;
				if (ep.function == "sensor") {
					ep.minvalue = 0;
					ep.maxvalue = 100;
				}
				if ((!devicetree.devices[dev.ip]
							|| !devicetree.devices[dev.ip].endpoints[ep.eid])
						&& !ignore_endpoint(dev.ip, ep.eid)) {
					add_endpoint(dev.ip, ep.eid, ep);
				}
			}
		}
	});
};

enumerate_network();

setInterval(save_devicetree, 10 * 60 * 1000);
setInterval(enumerate_network, 60 * 60 * 1000);
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
  app.use(express.urlencoded());
  app.use(express.json());
  app.use(express.methodOverride());
  app.use(app.router);
  app.use(express.static(path.join(application_root, "public")));
  app.use(express.errorHandler({ dumpExceptions: true, showStack: true }));
});


app.get('/api', function(req, res) {
  res.send("API is running.");
});

app.put('/api/sensor/:ip/:eid', function(req, res) {
	if (ignore_endpoint(req.params.ip, req.params.eid)) {
		res.send("Ignored", 200);
		return;
	}
	var device = devicetree.devices[req.params.ip];
	if (device && device.endpoints[req.params.eid]) {
		res.send("Sensor exists", 200);
	} else {
		try {
			var endpoint = add_endpoint(req.params.ip, req.params.eid, req.body);
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
	if (ignore_endpoint(req.params.ip, req.params.eid)) {
		res.send("Ignored", 200);
		return;
	}
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
		if (nconf.get('wizard_step')=='0') {
			res.render('index.ejs', {
				active_nav: 'dashboard',
				views: devicetree.views
			});
		} else {
			res.redirect('/wizard/new');
		}
});
app.get('/about', function(req, res) {
	res.render('about.ejs', { active_nav: 'about' });
});
app.get('/wizard', function(req, res) {
		if (nconf.get('wizard_step')=='0') {
			res.redirect('/wizard/current');
		} else {
			res.redirect('/wizard/new');
		}
});
app.get('/wizard/current', function(req, res) {
	var config = hexabus.read_current_config();
	hexabus.get_activation_code(function(err, activation_code) {
		config.activation_code = activation_code;

		hexabus.get_heartbeat_state(function(err, state) {
			config.active_nav = 'configuration';

			if (err) {
				config.heartbeat_ok = false;
				config.heartbeat_code = 0;
				config.heartbeat_messages = [err];
				config.heartbeat_state = "";
			} else {
				config.heartbeat_ok = state.code == 0;
				config.heartbeat_code = state.code;
				config.heartbeat_messages = state.messages;
				config.heartbeat_state = state;
			}

			res.render('wizard/current.ejs', config);
		});
	});
});
app.post('/wizard/reset', function(req, res) {
	try {
		fs.unlinkSync(devicetree_file);
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
	res.render('wizard/resetdone.ejs', { active_nav: 'configuration' });
	nconf.set('wizard_step', undefined);
	nconf.save();
	var wizard = new Wizard();
	wizard.complete_reset();
});
app.get('/wizard/devices', function(req, res) {
	var devices = {};

	devicetree.forEach(function(device) {
		var entry = devices[device.ip] = { name: device.name, ip: device.ip, eids: [] };

		device.forEachEndpoint(function(ep) {
			if (ep.function != "infrastructure") {
				entry.eids.push(ep);
			}
		});
		entry.eids.sort(function(a, b) {
			return b.eid - a.eid;
		});
	});

	res.render('wizard/devices.ejs', { active_nav: 'configuration', devices: devices });
});
app.get('/wizard/devices/add', function(req, res) {
	res.render('wizard/add_device.ejs', { active_nav: 'configuration' });
});
app.get('/wizard/:step', function(req, res) {
	// steps 1-... and 0 if completed, undefined if not started
	var wizard_step = nconf.get('wizard_step');
	var error = false;
	var getCurrentStep = function(wizard_step) {
		switch(wizard_step) {
			case undefined: return 'new';
			case '0': return 'reset';
			case '1': return 'connection';
			case '2': return 'activation';
			case '3': return 'msg';
			default: return 'reset';
		}
	}
	switch(req.params.step) {
		case "new":
			if(wizard_step!=undefined) {
				var current_step = getCurrentStep(wizard_step);
				res.render('wizard/' + current_step  + '.ejs', { active_nav: 'configuration', error: 'gt_error' });
			} else {
				nconf.set('wizard_step', '1');
				nconf.save();
				res.render('wizard/new.ejs', { active_nav: 'configuration' });
			}
			return;
		case "connection":
			var current_step = getCurrentStep(wizard_step);
			if(wizard_step==undefined) {
				nconf.set('wizard_step', '1');
				nconf.save();
				res.render('wizard/new.ejs', { active_nav: 'configuration', error: 'lt_error' });
			} else if(wizard_step>'1' || wizard_step=='0') {
				res.render('wizard/' + current_step  + '.ejs', { active_nav: 'configuration', error: 'gt_error' });
			} else {
				
				res.render('wizard/' + req.params.step  + '.ejs', { active_nav: 'configuration' });
			}
			return;
		case "activation":
			var current_step = getCurrentStep(wizard_step);
			if(wizard_step==undefined) {
				nconf.set('wizard_step', '1');
				nconf.save();
				res.render('wizard/new.ejs', { active_nav: 'configuration', error: 'lt_error' });
			} else if(wizard_step<'2') {
				res.render('wizard/' + current_step  + '.ejs', { active_nav: 'configuration', error: 'lt_error' });
			} else if(wizard_step>'2' || wizard_step=='0') {
				res.render('wizard/' + current_step  + '.ejs', { active_nav: 'configuration', error: 'gt_error' });
			} else {
				res.render('wizard/' + req.params.step  + '.ejs', { active_nav: 'configuration' });
			}
			return;
		case "msg":
			var current_step = getCurrentStep(wizard_step);
			if(wizard_step==undefined) {
				nconf.set('wizard_step', '1');
				nconf.save();
				res.render('wizard/new.ejs', { active_nav: 'configuration', error: 'lt_error' });
			} else if(wizard_step<'3') {
				res.render('wizard/' + current_step  + '.ejs', { active_nav: 'configuration', error: 'lt_error' });
			} else if(wizard_step>'3' || wizard_step=='0') {
				res.render('wizard/' + current_step  + '.ejs', { active_nav: 'configuration', error: 'gt_error' });
			} else {
				res.render('wizard/' + req.params.step  + '.ejs', { active_nav: 'configuration' });
			}
			return;
		case "success":
			var current_step = getCurrentStep(wizard_step);
			if(wizard_step==undefined) {
				nconf.set('wizard_step', '1');
				nconf.save();
				res.render('wizard/new.ejs', { active_nav: 'configuration', error: 'lt_error' });
			} else if(wizard_step<'3') {
				res.render('wizard/' + current_step  + '.ejs', { active_nav: 'configuration', error: 'lt_error' });
			} else if(wizard_step>'3' || wizard_step=='0') { // reset or display success page?
				res.render('wizard/' + current_step  + '.ejs', { active_nav: 'configuration', error: 'gt_error' });
			} else {
				nconf.set('wizard_step', '0');
				nconf.save();
				res.render('wizard/' + req.params.step  + '.ejs', { active_nav: 'configuration' });
			}
			return;
	}
	res.render('wizard/' + req.params.step  + '.ejs', { active_nav: 'configuration' });
});

app.get('/view/new', function(req, res) {
	var count = devicetree.views.length;

	devicetree.views.push({
		id: Math.round(Date.now() / 1000) + "." + count,
		name: "",
		devices: []
	});

	res.redirect('/view/edit/' + count);
});
app.get('/view/edit/:id', function(req, res) {
	var view = devicetree.views[req.params.id];

	if (!view) {
		res.send("view " + req.params.id + " not found", 404);
		return;
	}

	var devices = {};
	var used = view.devices;

	devicetree.forEach(function(device) {
		var entry = devices[device.ip] = { name: device.name, ip: device.ip, eids: [] };

		device.forEachEndpoint(function(ep) {
			if (ep.function != "infrastructure") {
				entry.eids.push(ep);
			}
		});
		entry.eids.sort(function(a, b) {
			return b.eid - a.eid;
		});
	});

	res.render('view/edit.ejs', {
		active_nav: 'configuration',
		known_devices: devices,
		used_devices: used,
		view_name: view.name,
		view_id: req.params.id
 	});
});
app.post('/view/edit/:id', function(req, res) {
	var view = devicetree.views[req.params.id];

	if (!view) {
		res.send("view not found", 404);
		return;
	}

	view.devices = JSON.parse(req.body.device_order);
	view.name = req.body.view_name;
	save_devicetree();

	res.redirect('/');
});

var sensor_is_old = function(ep) {
	return ep.age >= 60 * 60 * 1000;
};

setInterval(function() {
	devicetree.forEach(function(device) {
		device.forEachEndpoint(function(ep) {
			if (ep.function == "sensor" && sensor_is_old(ep)) {
				devicetree.emit('ep_timeout', { ep: ep });
			}
		});
	});
}, 1000);

io.sockets.on('connection', function (socket) {
  console.log("Registering new client.");

	var on = function(ev, cb) {
		socket.on(ev, function(data) {
			try {
				cb(data);
			} catch (e) {
				socket.emit('_error_', e);
			}
		});
	};

	var broadcast = function(ev, data) {
		socket.broadcast.emit(ev, data);
		socket.emit(ev, data);
	};

	var emit = socket.emit.bind(socket);

	var health_update_timeout;
	var send_health_update = function() {
		setTimeout(send_health_update, 60 * 1000);
		hexabus.get_heartbeat_state(function(err, state) {
			var wizard_step = nconf.get('wizard_step');
			emit('health_update', ((err || state.code != 0) && wizard_step == '0'));
		});
	};

	send_health_update();

	var broadcast_ep = function(ep) {
		broadcast('ep_metadata', ep);
	};
	var send_ep_update = function(ep) {
		emit('ep_update', { ep: ep.id, device: ep.device.ip, value: ep.last_value });
	};

	var devicetree_events = {
		endpoint_new_value: function(ep) {
			if (ep.function == "sensor" || ep.function == "actor") {
				send_ep_update(ep);
			}
		},
		endpoint_new: function(ep) {
			if (ep.function == "sensor" || ep.function == "actor") {
				emit('ep_new', ep);
			}
		},
		device_renamed: function(dev) {
			dev.forEachEndpoint(broadcast_ep);
		},
		ep_timeout: function(msg) {
			emit('ep_timeout', msg);
		}
	};

	for (var ev in devicetree_events) {
		devicetree.on(ev, devicetree_events[ev]);
	}
	on('disconnect', function() {
		for (var ev in devicetree_events) {
			devicetree.removeListener(ev, devicetree_events[ev]);
		}
		clearTimeout(health_update_timeout);
	});

	on('ep_request_metadata', function(id) {
		var ep = devicetree.endpoint_by_id(id);
		if (ep) {
			emit('ep_metadata', ep);
			if (ep.last_value != undefined) {
				send_ep_update(ep);
			}
		}
	});
	on('device_rename', function(msg) {
		var device = devicetree.devices[msg.device];
		if (!device) {
			emit('device_rename_error', { device: msg.device, error: 'Device not found' });
			return;
		}
		if (!msg.name) {
			emit('device_rename_error', { device: msg.device, error: 'No name given' });
			return;
		}

		hexabus.rename_device(msg.device, msg.name, function(err) {
			if (err) {
				emit('device_rename_error', { device: msg.device, error: err });
				return;
			}
			device.name = msg.name;
		});
	});
	on('ep_change_metadata', function(msg) {
		var ep = devicetree.endpoint_by_id(msg.id);
		if (ep) {
			["minvalue", "maxvalue"].forEach(function(key) {
				ep[key] = (msg.data[key] != undefined) ? msg.data[key] : ep[key];
			});
			save_devicetree();
			broadcast_ep(ep);
		}
	});

	on('ep_set', function(msg) {
		hexabus.write_endpoint(msg.ip, msg.eid, msg.type, msg.value, function(err) {
			if (err) {
				console.log(err);
			} else {
				var ep = devicetree.endpoint_by_id(msg.id);
				if (ep) {
					ep.last_value = {
						unix_ts: Math.round(Date.now() / 1000),
						value: msg.value
					};
				} else {
					console.log("Endpoint not found");
				}
			}
		});
	});

	on('wizard_configure', function() {
		var wizard = new Wizard();

		wizard.configure_network(function(progress) {
			emit('wizard_configure_step', progress);
		});
	});

	on('wizard_register', function() {
		var wizard = new Wizard();

		wizard.registerMSG(function(progress) {
			emit('wizard_register_step', progress);
		});
	});

	on('get_activation_code', function() {
		var wizard = new Wizard();

		wizard.getActivationCode(function(data) {
			emit('activation_code', data);
		});
	});

	on('devices_add', function() {
		var wizard = new Wizard();

		wizard.addDevice(devicetree, function(msg) {
			emit('device_found', msg);
		});
	});

	on('device_remove', function(msg) {
		try {
			devicetree.remove(msg.device);
			save_devicetree();
			broadcast('device_removed', msg);
		} catch (e) {
			console.log({ msg: msg, error: e });
		}
	});

	on('devices_enumerate', function() {
		enumerate_network(function() {
			emit('devices_enumerate_done');
		});
	});

	emit('clear_state');

	devicetree.forEach(function(device) {
		device.forEachEndpoint(function(ep) {
			emit('ep_metadata', ep);
			if (ep.last_value) {
				send_ep_update(ep);
			}
		});
	});
});
