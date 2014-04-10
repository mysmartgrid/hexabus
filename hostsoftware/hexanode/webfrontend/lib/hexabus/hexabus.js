'use strict';

var exec = require('child_process').exec;
var v6 = require('ipv6').v6;
var os = require('os');
var fs = require('fs');
var nconf = require('nconf');
var async = require('async');

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
		var program = nconf.get('debug-wizard')
			? '../backend/build/src/hexabus_msg_bridge --config bridge.conf -A'
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

	this.enumerate_network = function(cb) {
		cb = cb || Object;
		var interfaces = (nconf.get("debug-hxb-ifaces") || "eth0,usb0").split(",");
		interfaces.forEach(function(iface) {
			exec("hexinfo --discover --interface " + iface + " --json --devfile -", function(error, stdout, stderr) {
				if (error) {
					cb({ error: error });
				} else {
					var devices = JSON.parse(stdout).devices;
					devices.forEach(function(dev) {
						cb({ device: dev });
					});
					cb({ done: true });
				}
			});
		});
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

	this.master_slave_sm = function(msg, cb) {
		var sm_folder = 'state_machines/';
		var sm_build = sm_folder+'build/';
		
		exec('cp '+sm_folder+'master.hbh '+sm_folder+'master_slave.hbc '+sm_build, function(error, stdout, stderr) {
			if(!error) {
				var data=fs.readFileSync(sm_build+'master.hbh', { encoding: 'utf8' });
				data = data.replace('masterip', msg.master.ip);
				fs.writeFileSync(sm_build+'master.hbh', data, { encoding: 'utf8' });
				
				var ms_data = fs.readFileSync(sm_build+'master_slave.hbc', { encoding: 'utf8' });
				ms_data = ms_data.replace(/threshold/g, msg.threshold);
				var cmd = 'cp '+sm_folder+'slave.hbh '+sm_build+'slave0.hbh';
				for(var i=1; i<Object.keys(msg.slaves).length; i++) {
					cmd = cmd+' && cp '+sm_folder+'slave.hbh '+sm_build+'slave'+i+'.hbh';
				}
				exec(cmd, function(error, stdout, stderr) {
					if(!error) {
						var counter = 0;
						for(var slave in msg.slaves) {
							data = fs.readFileSync(sm_build+'slave'+counter+'.hbh', { encoding: 'utf8' });
							data = data.replace('device slave', 'device slave'+counter);
							data = data.replace('slaveip', msg.slaves[slave].ip);
							fs.writeFileSync(sm_build+'slave'+counter+'.hbh', data, { encoding: 'utf8' });

							ms_data = 'include slave'+counter+'.hbh;\n' + ms_data;
							ms_data = ms_data + 'instance slave_instance'+counter+' : slave_module (master, slave'+counter+', 0);\n';
							counter++;
						}
						fs.writeFileSync(sm_build+'master_slave.hbc', ms_data, { encoding: 'utf8' });
						exec('hbcomp '+sm_build+'master_slave.hbc -o '+sm_build+'tmp -d '+sm_folder+'datatypes.hb', function(error, stdout, stderr) {
							if(!error) {
								var asmfuns = [];
								asmfuns.push(function(callback) {
									exec('hbasm '+sm_build+'tmpmaster.hba -d '+sm_folder+'datatypes.hb -o '+sm_build+'master', function(error, stdout, stderr) {
									});
									callback();
								});
								var uplfuns = [];
								uplfuns.push(function(callback) {
									exec('hexaupload -p '+sm_build+'master', function(error, stdout, stderr) {
									});
									callback();
								});
								var pushFuns = function(i) {
									asmfuns.push(function(callback) {
										exec('hbasm '+sm_build+'tmpslave'+i+'.hba -d '+sm_folder+'datatypes.hb -o '+sm_build+'slave'+i, function(error, stdout, stderr) {
										});
										callback();
									});
									uplfuns.push(function(callback) {
										exec('hexaupload -p '+sm_build+'slave'+i, function(error, stdout, stderr) {
										});
										callback();
									});
								};
								for(var i=0; i<Object.keys(msg.slaves).length; i++) {
									pushFuns(i);
								}
								async.parallel(asmfuns, function(err) {
									if(!err) {
										async.series(uplfuns, function(err) {
											cb(true);
										});
									} else {
										cb(false, err);
									}
								});
							}
						});
					}
				});
			}
		});

	}
}

module.exports = hexabus;
