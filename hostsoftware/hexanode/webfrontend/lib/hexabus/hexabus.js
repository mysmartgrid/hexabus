'use strict';

var exec = require('child_process').exec;
var execFile = require('child_process').execFile;
var v6 = require('ipv6').v6;
var os = require('os');
var fs = require('fs-extra');
var path = require('path');
var nconf = require('nconf');
var async = require('async');
var ejs = require('ejs');
var es = require('event-stream');
var net = require('net');
var makeTempFile = require('mktemp').createFileSync;
var inherits = require('util').inherits;
var EventEmitter = require('events').EventEmitter;
var netroute = require('netroute');

var Hexabus = function() {
	this.rename_device = function(addr, newName, cb) {
		addr = new v6.Address(addr);
		if (!addr.valid) {
			cb({ error: "Invalid address" });
		} else {
			var command = "hexaupload --ip " + addr.canonicalForm() + " --rename '" + newName.replace("'", "''") + "'";
			exec(command, function(error, stdout, stderr) {
				if (error && cb) {
					cb(error);
				} else if (cb) {
					cb();
				}
			});
		}
	};

	this.read_current_config = function() {
		var ifaces = os.networkInterfaces();

		var gateway = netroute.getGateway();

		var nameservers = [];
		var resolvConf = fs.readFileSync("/etc/resolv.conf").toString();
		var lines = resolvConf.split("\n");
		for(var index in lines) {
			var line = lines[index].trim();
			if(line.indexOf("nameserver") === 0) {
				var nameserver = line.split(" ")[1];
				nameservers.push(nameserver);
			}
		}

		var config = {
			gateway: gateway,
			nameservers: nameservers,
			addrs: {
				lan: {
					v4: [],
					v6: [],
				},

				hxb: {
					v6: []
				}
			}
		};

		for (var dev in ifaces) {
			var target, read_v4;
			switch (dev) {
				case 'eth0':
					target = config.addrs.lan;
					read_v4 = true;
					break;

				case 'usb0':
					target = config.addrs.hxb;
					read_v4 = false;
					break;

				default:
					continue;
			}

			ifaces[dev].forEach(function(detail) {
				if (detail.family == 'IPv6') {
					target.v6.push(detail.address);
				} else if (detail.family == 'IPv4' && read_v4) {
					target.v4.push(detail.address);
				}
			});
		}

		return config;
	};

	this.get_activation_code = function(cb) {
		var program = nconf.get('debug-wizard') ? '../backend/build/src/hexabus_msg_bridge --config bridge.conf -A'
			: 'hexabus_msg_bridge -A';
		exec(program, function(error, stdout, stderr) {
			if (error) {
				cb(error, undefined);
			} else {
				cb(undefined, stdout);
			}
		});
	};

	this.get_heartbeat_state = function(cb) {
		fs.readFile('/var/run/hexabus_msg_heartbeat.state', { encoding: 'utf8' }, function(err, data) {
			if (err) {
				cb(err, undefined);
			} else {
				var lines = data.split('\n');
				lines = lines.slice(0, -1);
				var messages = lines.slice(0, -1);
				var ret = parseInt(lines[lines.length - 1]);
				cb(undefined, {
					code: ret,
					messages: messages
				});
			}
		});
	};

	var hexinfo_lock = false;

	this.enumerate_network = function(deviceCb) {
		deviceCb = deviceCb || Object;

		if(hexinfo_lock) {
			deviceCb({'error' : 'Hexinfo is already running, try again later ...'});
			return;
		}

		hexinfo_lock = true;

		var tmpEpFile = makeTempFile("/tmp/XXXXXX_epfile.tmp");
		var tmpDevFile = makeTempFile("/tmp/XXXXXX_devfile.tmp");
		fs.copySync(path.join(nconf.get("data"), 'endpoints.hbh'), tmpEpFile);
		fs.copySync(path.join(nconf.get("data"), 'devices.hbh'), tmpDevFile);

		console.log(tmpEpFile);
		console.log(tmpDevFile);
		
		var enumerateInterface = function(iface, cb) {
			execFile("hexinfo",
					["--discover",
					"--interface", iface,
					"--json", "-",
					"--epfile", tmpEpFile,
					"--devfile", tmpDevFile],
					function(error, stdout, stderr) {
				if (error) {
					cb({ error: error });
				} else {
					var devices = JSON.parse(stdout).devices;
					console.log('Hexinfo ' +iface + ':');
					console.log(devices);
					devices.forEach(function(dev) {
						deviceCb({ device: dev });
					});
					cb();
				}
			});
		};

		var interfaces = (nconf.get("debug-hxb-ifaces") || "eth0,usb0").split(",");
		async.each(interfaces, enumerateInterface, function(error) {
			hexinfo_lock = false;

			if(error !== undefined) {
				fs.unlinkSync(tmpEpFile);
				fs.unlinkSync(tmpDevFile);
				deviceCb({'error' : error});
			}
			else {
				fs.renameSync(tmpEpFile, path.join(nconf.get("data"), 'endpoints.hbh'));
				fs.renameSync(tmpDevFile, path.join(nconf.get("data"), 'devices.hbh'));
				deviceCb({'done' : true});
			}
		});
	};

	this.is_ignored_endpoint = function(eid) {
		return eid >= 7 && eid <= 12;
	};

	this.update_devicetree = function(devicetree, cb) {
		this.enumerate_network(function(dev) {
			if (dev.done !== undefined) {
				cb();
			} else if (dev.error !== undefined) {
				console.log(dev.error);
				cb(dev.error);
			} else {
				dev = dev.device;

				//Create the device if it does not exist
				if(devicetree.devices[dev.ip] === undefined) {
					devicetree.addDevice(dev.ip, dev.name, dev.sm_name);
				}
				else {
					if(devicetree.devices[dev.ip].name !== dev.name) {
						devicetree.devices[dev.ip].name = dev.name;
					}
				}

				for(var index in dev.endpoints) {
					var ep = dev.endpoints[index];
					if(devicetree.devices[dev.ip].endpoints[ep.eid] === undefined) {
						devicetree.devices[dev.ip].addEndpoint(ep.eid, ep); 
					}
				}
			}
		}.bind(this));
	};

	this.write_endpoint = function(ip, eid, type, value, callback) {
		var command = {'command' : 'send',
						'to' : {
							'ip' : ip,
							'port' : 61616,
						},
						'packet': {
							'type' : 'write',
							'eid' : eid,
							'datatype' : type,
							'value' : value
						}
					};

		if(juiceConnection !== null) {
			juiceConnection.write(JSON.stringify(command) + '\n');
			callback();
		}
		else {
			callback('Not connected to hexajuice');
		}
	};

	var juiceConnection = null;

	this.connect = function(callback) {
		var juiceSocket = '/tmp/hexajuice.sock';

		var hexajuiceServer = net.createServer(function(connection) {
			if(juiceConnection === null) {
				connection
				.pipe(es.split())
				.pipe(es.map(function(packet, mapCb) {
					try {
						var json = JSON.parse(packet);
						if(json.error === undefined) {
							this.emit('packet', json);
							console.log(json);
						}
						else {
							console.log(json);
						}
					}
					catch(ex) {
						console.log('Exception while parsing json: ' + ex);
						console.log(ex.stack);
					}
					mapCb();
				}.bind(this)));

				juiceConnection = connection;
				juiceConnection.on('close', function() {
					juiceConnection = null;
				});

				callback();
			}
		}.bind(this));

		if(fs.existsSync(juiceSocket)) {
			fs.unlinkSync(juiceSocket);
		}

		hexajuiceServer.listen(juiceSocket, function() {
			process.on('exit', function() {
				console.log('Closing socket');
				hexajuiceServer.close();
			});
		});
	};


	this.listen = function(iface) {
		var command = {'command' : 'listen', 'interface' : iface};
		if(juiceConnection !== null) {
			juiceConnection.write(JSON.stringify(command) + '\n');
		}
	};

	this.updateEndpointValues = function(devicetree) {

		this.on('packet', function(packet) {
			console.log(packet);
			packet = packet.packet;
			if(packet.type == 'info') {
				if(devicetree.devices[packet.from.ip] === undefined ||
					devicetree.devices[packet.from.ip].endpoints[packet.eid] === undefined) {

					console.log('Unknown device or endpoint:' + packet.from + ' ' + packet.eid);

					this.update_devicetree(devicetree,function(error) {
						if(error !== undefined) {
							console.log(error);
							console.log('Could not update device tree ...');
						}
					});
				}
				else {
					devicetree.devices[packet.from.ip].endpoints[packet.eid].last_value = packet.value;
				}
			}
		}.bind(this));
	};
};
inherits(Hexabus, EventEmitter);



module.exports = Hexabus;
