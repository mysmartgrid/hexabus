var application_root = __dirname, 
	path=require('path'),
	express=require('express'),
	connect = require('connect'),
	validator = require('validator'),
	app = module.exports = express(),
	server=require("http").createServer(app),
	io = require('socket.io').listen(server),
	DeviceTree = require("./lib/devicetree"),
	Hexabus = require("./lib/hexabus"),
	hexabus = new Hexabus(),
	Wizard = require("./lib/wizard"),
	statemachines = require("./lib/statemachines"),
	ApiController = require("./controllers/api"),
	WizardController = require("./controllers/wizard"),
	ViewsController = require("./controllers/views"),
	StatemachineController = require("./controllers/statemachine"),
	fs = require("fs"),
	nconf=require('nconf');

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
						&& !hexabus.is_ignored_endpoint(dev.ip, ep.eid)) {
					devicetree.add_endpoint(dev.ip, ep.eid, ep);
				}
			}
		}
	});
};

enumerate_network();

setInterval(enumerate_network, 60 * 60 * 1000);

var save_devicetree = function(cb) {
	devicetree.save(devicetree_file, cb);
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
  app.use(express.urlencoded());
  app.use(express.json());
  app.use(express.methodOverride());
  app.use(app.router);
  app.use(express.static(path.join(application_root, "public")));
  app.use(express.errorHandler({ dumpExceptions: true, showStack: true }));
});

ApiController.expressSetup(app, nconf, hexabus, devicetree);
WizardController.expressSetup(app, nconf, hexabus, devicetree);
ViewsController.expressSetup(app, nconf, hexabus, devicetree);
StatemachineController.expressSetup(app, nconf, hexabus, devicetree);

app.get('/', function(req, res) {
		var wizard = new Wizard();
		if (wizard.is_finished()) {
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





var sensor_is_old = function(ep) {
	return ep.age >= 60 * 1000; //60 * 60 * 1000;
};

setInterval(function() {
	devicetree.forEach(function(device) {
		device.forEachEndpoint(function(ep) {
			if (ep.function == "sensor" && ep.eid != 4 && ep.eid != 1 && sensor_is_old(ep)) {
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
		health_update_timeout = setTimeout(send_health_update, 60 * 1000);
		var wizard = new Wizard();
		hexabus.get_heartbeat_state(function(err, state) {
			emit('health_update', ((err || state.code != 0) && wizard.is_finished()));
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
			/*if (ep.last_value != undefined) {
				send_ep_update(ep);
			}*/
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

	on('upgrade', function() {
		var wizard = new Wizard();

		wizard.upgrade(function(error) {
			emit('upgrade_completed', { msg: error });
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

	var busy = false;

	var sendError = function(error) {
		emit('sm_uploaded', {sucess: false, error: error});
	}

	var buildStatemachine = function(msg, statemachineModule) {
		console.log(msg);
		if(!busy) {
			
			console.log("Validating data");

			var error = statemachineModule.validateInput(msg);
			console.log(error);
			if(error) {
				console.log("Validation failed");
				console.log(error);
				sendError(error);
				return;
			}

			console.log("Data passed validation");

			busy = true;
			statemachineModule.buildMachine(msg,
				function(msg) {
					emit('sm_progress', msg);
				},
				function(success, error) {
					console.log(error);
					busy = false;
					emit('sm_uploaded', {success: success, error: error});
			});
		}
	}

	on('master_slave_sm', function(msg) {
		buildStatemachine(msg,statemachines.masterSlave);
	});

	on('standbykiller_sm', function(msg) {
		buildStatemachine(msg,statemachines.standbyKiller);
	});

	on('productionthreshold_sm', function(msg) {
		buildStatemachine(msg,statemachines.productionThreshold);
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
