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
	WizardController = require("./controllers/wizard"),
	ViewsController = require("./controllers/views"),
	HexabusServer = require("./controllers/hexabusserver"),
	StatemachineController = require("./controllers/statemachine"),
	fs = require("fs"),
	debug = require('./lib/debug'),
	nconf=require('nconf');

nconf.env().argv().file({file: 'config.json'});

// Setting default values
nconf.defaults({
	'debug' : false,
	'port': '3000',
	'data': 'data',
	'hxb-interfaces' : 'usb0,eth0'
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




// Load Devicetree
var dataDir = nconf.get('data');
if(!fs.existsSync(dataDir)) {
	fs.mkdir(dataDir);
}

var devicetree_file = path.join(dataDir,'devicetree.json');

var devicetree;
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


// Set up timer to regularily enumerate the network
var enumerate_network = function() {
	hexabus.update_devicetree(devicetree, function(error) {
		if(error !== undefined) {
			console.log('Error updating devicetree');
			console.log(error);
		}
	});
};
enumerate_network();
setInterval(enumerate_network, 60 * 60 * 1000);

hexabus.connect(function() {
	debug('Got connection, trying to listen');
	nconf.get('hxb-interfaces').split(',').forEach(hexabus.listen.bind(hexabus));
});
hexabus.updateEndpointValues(devicetree);

// Setup timer to regularily save the devictree to disk (just in case)
var save_devicetree = function(cb) {
	fs.writeFile(devicetree_file, JSON.stringify(devicetree, null, '\t'), function(err) {
		if (cb) {
			cb(err);
		}
	});
	debug('Saved Devicetree');
};
setInterval(save_devicetree, 2 * 60 * 1000);

// We also need to save the devicetree if we are killed
process.on('SIGINT', function() {
	save_devicetree(process.exit);
});

process.on('SIGTERM', function() {
	save_devicetree(process.exit);
});



console.log("Using configuration: ");
console.log(" - port: " + nconf.get('port'));
console.log(" - data dir: " + nconf.get('data'));
console.log(" - hxb-interfaces: " + nconf.get('hxb-interfaces'));
console.log(" - debug: " + nconf.get('debug'));


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

/*
 * Initialize the different controllers.
 */
WizardController.expressSetup(app, nconf, hexabus, devicetree);
ViewsController.expressSetup(app, nconf, hexabus, devicetree);
StatemachineController.expressSetup(app, nconf, hexabus, devicetree);

/*
 * The overview page.
 * This page may redirect to /wizzard/new in case hexanode has not yet been configured.
 */
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


/*
 * The about page for displaying some rudimentary version info.
 */
app.get('/about', function(req, res) {
	res.render('about.ejs', { active_nav: 'about' });
});


/*
 * Make the devicetree.js file accessable to the browser.
 * Note: The file is written in special way to be executed by nodejs as well as by a browsers javascript engine.
 */
app.get('/commonjs/devicetree.js', function(req, res) {
	res.sendfile(path.join(application_root, 'lib/devicetree/devicetree.js'));
});


/*
 * Socket.io setup
 */
io.on('connection', function (socket) {
	debug("Registering new client.");

	// Simple wrapper around socket.on that provides better exception handling.
	var on = function(ev, cb) {
		socket.on(ev, function(data) {
			try {
				cb(data);
			} catch (e) {
				console.log('error in socket.io handler: ' + e);
				socket.emit('_error_', e);
			}
		});
	};

	/*
	 * Since socket.broadcast sends a message to every socket except the one it is called on,
	 * this wrapper is required to do a real broadcast.
	 */
	var broadcast = function(ev, data) {
		socket.broadcast.emit(ev, data);
		socket.emit(ev, data);
	};

	// Shorthand for socket.emit
	var emit = socket.emit.bind(socket);

	// A hexabusserver to handle hexabus_... messages
	var hexabusServer = new HexabusServer(socket, hexabus, devicetree);

	// Setup handlers for statemachine messages
	StatemachineController.socketioSetup(socket, hexabus, devicetree);

	// Setup handler needed by the wizzard pages
	WizardController.socketioSetup(on, emit, hexabus, devicetree);


	/*
	 * Setup handlers for devicetree synchronization.
	 *
	 * Since the devicetree aims to be agnostic towards the communication channel
	 * used to synchronize its instances, there is no specialized setup method for socket.io.
	 */

	/*
	 * Send the full devicetree to initialize a new browserside instance.
	 */
	on('devicetree_request_init', function() {
		emit('devicetree_init', devicetree.toJSON());
	});

	/*
	 * Apply a browerside update to the serverside instance and propagate it to all other browserside instances.
	 *
	 * If applying the update locally fails the update is not propagated.
	 * Instead an error message is send backt to the browserside instance.
	 */
	socket.on('devicetree_update', function(update) {
		try {
			devicetree.applyUpdate(update);
			socket.broadcast.emit('devicetree_update', update);
		}
		catch(error) {
			socket.emit('devicetree_error', error);
		}
	});


	/*
	 * Apply a browserside deletion to the serverside instance and propagate it to all other browserside instances.
	 *
	 * If applying the deletion locally fails the deletion is not propagated.
	 * Instead an error message is send backt to the browserside instance.
	 */
	socket.on('devicetree_delete', function(update) {
		try {
			devicetree.applyDeletion(update);
			socket.broadcast.emit('devicetree_delete', update);
		}
		catch(error) {
			socket.emit('devicetree_error', error);
		}
	});


	/*
	 * Propagate a serverside devicetree update to all browserside instances.
	 */
	devicetree.on('update',function(update) {
		emit('devicetree_update', update);
	});

	/*
	 * Propagate a serverside devicetree deletion to all browserside instances.
	 */
	devicetree.on('delete',function(deletion) {
		emit('devicetree_delete', deletion);
	});


	/*
	 * Send an update for the mysmartgrid heartbeat state to browser.
	 */
	var health_update_timeout;
	var send_health_update = function() {
		health_update_timeout = setTimeout(send_health_update, 60 * 1000);
		var wizard = new Wizard();
		hexabus.get_heartbeat_state(function(err, state) {
			emit('health_update', ((err || state.code !== 0) && wizard.is_finished()));
		});
	};
	send_health_update();

	// Reset the browserside sate
	emit('clear_state');
});
