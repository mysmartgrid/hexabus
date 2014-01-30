var nconf = require('nconf');
var exec = require('child_process').exec;
var DeviceTree = require("../devicetree")
var v6 = require('ipv6').v6;

var Wizard = function() {
	var networkAutoconf = function(cb,steps) {
		var command = nconf.get('debug-wizard')
			? 'sleep 2'
			: 'sudo hxb-net-autoconf init';
		exec(command, function(error, stdout, stderr) {
			if (error) {
				cb({ step: 'autoconf', error: error });
			} else {
				steps.nA=true
				check_configure_state(steps)
				cb({ step: 'autoconf', error: undefined });
			}
		});
	};

	var checkMSG = function(cb,steps) {
		exec('ping -c4 mysmartgrid.de', function(error, stdout, stderr) {
			if (error) {
				cb({ step: 'check_msg', error: error });
			} else {
				steps.cM=true
				check_configure_state(steps)
				cb({ step: 'check_msg', error: undefined });
			}
		});
	};

	check_configure_state = function(steps) {
		if(steps.nA && steps.cM) {
			nconf.set('wizard_step', '2');
			nconf.save()
		}
	}

	this.configure_network = function(cb) {
		var steps = new Object();
		steps.nA = false;
		steps.cM = false;
		networkAutoconf(cb, steps);
		checkMSG(cb, steps);
	};

	this.deconfigure_network = function(cb) {
		var command = nconf.get('debug-wizard')
			? 'sleep 0'
			: 'sudo hxb-net-autoconf forget';
		exec(command, cb);
	};

	this.complete_reset = function() {
		var command = nconf.get('debug-wizard')
			? 'sleep 0'
			: 'sleep 5 && sudo reboot';
		exec(command, function() {});
	};

	this.registerMSG = function(cb) {
		var program = nconf.get('debug-wizard')
			? '../backend/build/src/hexabus_msg_bridge --config bridge.conf'
			: 'sudo hexabus_msg_bridge';
		var register_command = nconf.get('debug-wizard')
			? program + ' --create https://dev3-api.mysmartgrid.de:8443'
			: program + ' --create';
		var retrieve_command = program + ' --activationcode';
		exec(register_command, function(error, stdout, stderr) {
			if (error) {
				cb({ step: 'register_msg', error: error });
			} else {
				cb({ step: 'register_msg', error: undefined });
				exec(retrieve_command, function(error, stdout, stderr) {
					if (error) {
						cb({ step: 'register_code', error: error });
					} else {
						nconf.set('wizard_step', '3');
						nconf.save()
						cb({ step: 'register_code', error: undefined, code: stdout });
					}
				});
			}
		});
	};

	this.unregisterMSG = function(cb) {
		var script = nconf.get('debug-wizard')
			? 'rm -f bridge.conf'
			: 'sudo rm -f /etc/hexabus_msg_bridge.conf; sudo svc -k /etc/service/hexabus_msg_bridge';
		exec(script, cb);
	};

	this.getActivationCode = function(cb) {
		var program = nconf.get('debug-wizard')
			? '../backend/build/src/hexabus_msg_bridge --config bridge.conf'
			: 'sudo hexabus_msg_bridge';
		var retrieve_command = program + ' --activationcode';
		exec(retrieve_command, function(error, stdout, stderr) {
			if (error) {
				cb({ error: error });
			} else {
				cb({ error: undefined, code: stdout });
			}
		});
	};

	this.addDevice = function(devicetree, cb) {
		var script = nconf.get('debug-wizard')
			? 'sleep 1 && cat tools/device.json'
			: 'hexapair -D /dev/ttyACM0 -t 30';
		exec(script, function(error, stdout, stderr) {
			if (error) {
				cb({ device: stdout, error: stderr });
			} else {
				var interfaces = (nconf.get("debug-hxb-ifaces") || "eth0,usb0").split(",");
				var addr = new v6.Address(stdout.replace(/\s+$/g, ''));
				interfaces.forEach(function(iface) {
					var command = nconf.get('debug-wizard')
						? 'echo \''+stdout+'\''
						: 'hexinfo --interface ' + iface + ' --ip ' + addr.canonicalForm() + ' --json --devfile -';
					exec(command, function(error, stdout, stderr) {
						if (error) {
							cb({ error: stderr });
						} else {
							var dev = JSON.parse(stdout);
							if(dev && dev.devices && dev.devices[0] && dev.devices[0].endpoints) {
								dev = dev.devices[0];
								for (var key in dev.endpoints) {
									var ep = dev.endpoints[key];
									ep.name = ep.name || dev.name;
									if (ep.function == "sensor") {
										ep.minvalue = 0;
										ep.maxvalue = 100;
									}
									if (!devicetree.devices[dev.ip]
										|| !devicetree.devices[dev.ip].endpoints[ep.eid]) {
										devicetree.add_endpoint(dev.ip, ep.eid, ep);
									}
								}
								cb({ device: devicetree.devices[dev.ip], error: undefined });
							} else {
								cb({error: "Error while querying device"});
							}
						}
					});
				});
			}
		});
	};
};

module.exports = Wizard;
