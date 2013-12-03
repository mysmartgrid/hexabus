var nconf = require('nconf');
var exec = require('child_process').exec;
var DeviceTree = require("../devicetree")
var v6 = require('ipv6').v6;

var Wizard = function() {
	var networkAutoconf = function(cb) {
		var command = nconf.get('debug-wizard')
			? 'sleep 2'
			: 'sudo hxb-net-autoconf init';
		exec(command, function(error, stdout, stderr) {
			if (error) {
				cb({ step: 'autoconf', error: error });
			} else {
				cb({ step: 'autoconf', error: undefined });
			}
		});
	};

	var checkMSG = function(cb) {
		exec('ping -c4 mysmartgrid.de', function(error, stdout, stderr) {
			if (error) {
				cb({ step: 'check_msg', error: error });
			} else {
				cb({ step: 'check_msg', error: undefined });
			}
		});
	};

	this.configure_network = function(cb) {
		networkAutoconf(cb);
		checkMSG(cb);
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
