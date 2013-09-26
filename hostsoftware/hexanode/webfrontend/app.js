var application_root = __dirname
  , path=require('path')
  , express=require('express')
  , connect = require('connect')
  , app = module.exports = express()
  , server=require("http").createServer(app)
  , io = require('socket.io').listen(server)
  , Cache=require("./lib/sensorcache")
	, hexabus=require("./lib/hexabus")
  , nconf=require('nconf');

nconf.env().argv();
// Setting default values
nconf.defaults({
  'port': '3000',
  'server': '10.23.1.250'
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

var sensorcache = new Cache();
console.log("Using configuration: ");
console.log(" - server: " + nconf.get('server'));
console.log(" - port: " + nconf.get('port'));


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
	res.send(JSON.stringify(sensorcache.get_sensor_info_list()));
});

app.get('/api/sensor/:id/latest', function(req, res) {
	var result = sensorcache.get_last_value(req.params.id);
	if (result) {
		res.send(JSON.stringify(result));
	} else {
		res.send("Sensor " + req.params.id + " not found", 404);
	}
});

app.get('/api/sensor/:id', function(req, res) {
	var sensor = sensorcache.get_sensor_info(req.params.id);
	if (sensor) {
		res.send(JSON.stringify(sensor));
	} else {
		res.send("Sensor " + req.params.id + " not found", 404);
	}
});

app.put('/api/sensor/:id', function(req, res) {
	try {
		var id = req.params.id;
		var sensor = sensorcache.add_sensor(id, req.body);
		res.send("New sensor " + id + " added");
	} catch(err) {
		res.send(err, 422);
	}
});

app.post('/api/sensor/:id', function(req, res) {
	try {
		sensorcache.add_value(req.params.id, req.body.value);
		res.send("OK");
	} catch(err) {
		if (err == "Sensor not found") {
			res.send("Sensor " + req.params.id + " not found", 404);
		} else if (err == "Value required") {
			res.send(err, 422);
		} else {
			res.send(err, 500);
		}
	}
});

app.post('/api/device/rename/:ip', function(req, res) {
	try {
		if (typeof req.body.name != "string") {
			res.send("No name given", 400);
		}
		new hexabus().rename_device(req.params.ip, req.body.name, function() {
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
  sensorcache.on('sensor_update', function(msg) {
    socket.volatile.emit('sensor_update', { sensor: msg });
  });
	sensorcache.on('sensor_new', function(sensor) {
		socket.volatile.emit('sensor_new', sensor);
	});
	socket.on('sensor_request_metadata', function(sensor_id) {
		socket.volatile.emit('sensor_metadata', sensorcache.get_sensor_info(sensor_id));
	});
  // a client can also initiate a data update.
  socket.on('sensor_refresh_data', function() {
  //  console.log("Refresh data event.");
    sensorcache.enumerate_current_values(function(msg) {
      socket.volatile.emit('sensor_update', { sensor: msg });
    });
  });
	socket.on('device_rename', function(msg) {
		if (sensorcache.get_sensor_info(msg.device)) {
			try {
				console.log(msg.device.replace(/_.*/g, ""));
				new hexabus().rename_device(msg.device.replace(/_.*/g, ""), msg.newName, function() {
					sensorcache.remove_sensor(msg.device);
					socket.emit('device_rename_error', null);
				});
			} catch (err) {
				socket.emit('device_rename_error', err);
			}
		} else {
			socket.emit('device_rename_error', "No such device");
		}
	});

	socket.emit('clear_state');

	var list = sensorcache.get_sensor_info_list();
	for (sensor in list) {
		socket.emit('sensor_metadata', list[sensor]);
		socket.emit('sensor_update', { sensor: sensorcache.get_last_value_info(list[sensor].id) });
	}
});

