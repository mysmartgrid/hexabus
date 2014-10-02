var application_root = __dirname, 
	path=require('path'),
	express=require('express'),
	connect = require('connect'),
	validator = require('validator'),
	app = module.exports = express(),
	server=require('http').Server(app),
	io = require('socket.io')(server),
	DeviceTree = require("./lib/devicetree"),
	Hexabus = require("./lib/hexabus"),
	hexabus = new Hexabus(),
	Wizard = require("./lib/wizard"),
	statemachines = require("./lib/statemachines"),
	ApiController = require("./controllers/api"),
	WizardController = require("./controllers/wizard"),
	ViewsController = require("./controllers/views"),
	HexabusServer = require("./controllers/hexabusserver"),
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

var loadDeviceTree = function() {
	try {
		if(fs.existsSync(devicetree_file)) {
			var jsonTree = JSON.parse(fs.readFileSync(devicetree_file));
			devicetree = new DeviceTree(jsonTree);
		}
		else {
			throw 'File: ' + devicetree_file + ' does not exist';
		}
	}
	catch(e) {
		console.log(e);
		devicetree = new DeviceTree();
	}
};

loadDeviceTree();

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
				if ((!devicetree.devices[dev.ip] || !devicetree.devices[dev.ip].endpoints[ep.eid]) && !hexabus.is_ignored_endpoint(dev.ip, ep.eid)) {
					devicetree.add_endpoint(dev.ip, ep.eid, ep);
				}
				else if(devicetree.devices[dev.ip] && devicetree.devices[dev.ip].endpoints[ep.eid] && !hexabus.is_ignored_endpoint(dev.ip, ep.eid)) {
					devicetree.devices[dev.ip].endpoints[ep.eid].update();
				}
			}
		}
	});
};

enumerate_network();

setInterval(enumerate_network, 60 * 60 * 1000);

var save_devicetree = function(cb) {
	fs.writeFile(devicetree_file, JSON.stringify(devicetree, null, '\t'), function(err) {
		if (cb) {
			cb(err);
		}
	});
	console.log('Saved Devicetree');
};

setInterval(save_devicetree, 2 * 60 * 1000);

process.on('SIGINT', function() {
	save_devicetree(process.exit);
});

process.on('SIGTERM', function() {
	save_devicetree(process.exit);
});

console.log("Using configuration: ");
console.log(" - port: " + nconf.get('port'));
console.log(" - config dir: " + nconf.get('config'));

// see http://stackoverflow.com/questions/4600952/node-js-ejs-example
// for EJS
app.configure(function () {
  app.set('views', __dirname + '/views');
  app.set('view engine', 'ejs');
//	app.use(connect.logger('dev'));
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

app.get('/commonjs/devicetree.js', function(req, res) {
	res.sendfile(path.join(application_root, 'lib/devicetree/devicetree.js'));
});

io.on('connection', function (socket) {
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

	var hexabusServer = new HexabusServer(socket, devicetree);

	StatemachineController.socketioSetup(emit, on, hexabus, devicetree);
	WizardController.socketioSetup(on, emit, devicetree);


	on('devicetree_request_init', function() {
		emit('devicetree_init', devicetree.toJSON());
	});

	socket.on('devicetree_update', function(update) {
		try {
			devicetree.applyUpdate(update);
			socket.broadcast.emit('devicetree_update', update);
		}
		catch(error) {
			socket.emit('devicetree_error', error);
		}
	});

	socket.on('devicetree_delete', function(update) {
		try {
			devicetree.applyDeletion(update);
			socket.broadcast.emit('devicetree_delete', update);
		}
		catch(error) {
			socket.emit('devicetree_error', error);
		}
	});


	devicetree.on('update',function(update) {
		emit('devicetree_update', update);
	});

	devicetree.on('delete',function(deletion) {
		emit('devicetree_delete', deletion);
	});

	var health_update_timeout;
	var send_health_update = function() {
		health_update_timeout = setTimeout(send_health_update, 60 * 1000);
		var wizard = new Wizard();
		hexabus.get_heartbeat_state(function(err, state) {
			emit('health_update', ((err || state.code !== 0) && wizard.is_finished()));
		});
	};

	send_health_update();

	
	emit('clear_state');
});
