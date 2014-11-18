'use strict';

var exec = require('child_process').exec;
var v6 = require('ipv6').v6;
var os = require('os');
var fs = require('fs');
var nconf = require('nconf');
var async = require('async');
var ejs = require('ejs');
var es = require('event-stream');
var net = require('net');

var hexabus = function() {
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

		var config = {
			addrs: {
				lan: {
					v4: [],
					v6: []
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
		
		var enumerateInterface = function(iface, cb) {
			exec("hexinfo --discover --interface " + iface + " --json -", function(error, stdout, stderr) {
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

		if(hexinfo_lock) {
			deviceCb({'error' : 'Hexinfo is already running, try agaian later ...'});
		}
		
		hexinfo_lock = true;

		var interfaces = (nconf.get("debug-hxb-ifaces") || "eth0,usb0").split(",");
		async.each(interfaces, enumerateInterface, function(error) {
			if(error !== undefined) {
				hexinfo_lock = false;
				deviceCb({'error' : error});
			}
			else {
				hexinfo_lock = false;
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


	this.read_endpoint = function(ip, eid, cb) {
	};

	this.write_endpoint = function(ip, eid, type, value, cb) {
		var command = "hexaswitch set " + ip + " --eid " + eid + " --datatype " + type + " --value ";
		// type 1 is bool
		if (type == 1) {
			command = command + (value ? "1" : "0");
		}	else {
			command = command + value;
		}

		exec(command, function(error, stdout, stderr) {
			cb(error);
		});
	};

	this.listen = function(iface, cb) {
		var hexaswitchServer = net.createServer(function(connection) {
			connection
			.pipe(es.split())
			.pipe(es.map(function(packet, mapCb) {
				try {
					var json = JSON.parse(packet);
					cb(json);
				}
				catch(ex) {
					console.log('Exception while parsing json: ' + ex);
					console.log(ex.stack);
				}
				mapCb();
			}));
		});

		if(fs.existsSync('/tmp/hexaswitch.sock')) {
			fs.unlinkSync('/tmp/hexaswitch.sock');
		}

		hexaswitchServer.listen('/tmp/hexaswitch.sock', function() {
			process.on('exit', function() {
				console.log('Closing socket');
				hexaswitchServer.close();
			});
		});

	};

	this.updateEndpointValues = function(devicetree) {

		this.listen('usb0', function(packet) {
			//console.log(packet);
			if(packet.type == 'Info') {
				if(devicetree.devices[packet.from] === undefined ||
					devicetree.devices[packet.from].endpoints[packet.eid] === undefined) {

					console.log('Unknown device or endpoint:' + packet.from + ' ' + packet.eid);

					this.update_devicetree(devicetree,function(error) {
						if(error !== undefined) {
							console.log(error);
							console.log('Could not update device tree ...');
						}
					});
				}
				else {
					devicetree.devices[packet.from].endpoints[packet.eid].last_value = packet.value;
				}
			}
		}.bind(this));
	};
};

module.exports = hexabus;
