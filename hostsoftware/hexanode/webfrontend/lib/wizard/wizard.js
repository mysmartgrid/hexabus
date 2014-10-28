var nconf = require('nconf');
var exec = require('child_process').exec;
var DeviceTree = require("../devicetree");
var v6 = require('ipv6').v6;
var fs = require("fs");
var async = require("async");

var Wizard = function() {
	var networkAutoconf = function(cb,steps) {
		var command = nconf.get('debug-wizard') ? 'sleep 2' : 'sudo hxb-net-autoconf init';
		exec(command, function(error, stdout, stderr) {
			if (error) {
				cb({ step: 'autoconf', error: error });
			} else {
				steps.nA = true;
				check_configure_state(steps);
				cb({ step: 'autoconf', error: undefined });
			}
		});
	};

	var checkMSG = function(cb,steps) {
		exec('ping -c4 mysmartgrid.de', function(error, stdout, stderr) {
			if (error) {
				cb({ step: 'check_msg', error: error });
			} else {
				steps.cM = true;
				check_configure_state(steps);
				cb({ step: 'check_msg', error: undefined });
			}
		});
	};

	var check_configure_state = function(steps) {
		if(steps.nA && steps.cM) {
			nconf.set('wizard_step', '2');
			nconf.save();
		}
	};

	this.is_finished = function() {
		var step = nconf.get('wizard_step');
		if((step === undefined) && fs.existsSync('/etc/hexabus_msg_bridge.conf')) {
			nconf.set('wizard_step', '0');
			nconf.save();
			return true;
		}
		return (step == '0');	
	};

	this.upgrade = function(cb) {
		var command = nconf.get('debug-wizard') ? 'apt-get -s upgrade' : 'sudo apt-get update && sudo apt-get -s upgrade && sudo apt-get -y upgrade';
		exec(command, function(error, stdout, stderr) {
			if(error) {
				cb( stderr );
			} else {
				cb( undefined );
			}
		});
	};

	this.configure_network = function(cb) {
		var steps = {};
		steps.nA = false;
		steps.cM = false;
		networkAutoconf(cb, steps);
		checkMSG(cb, steps);
	};

	this.deconfigure_network = function(cb) {
		var command = nconf.get('debug-wizard') ? 'sleep 0' : 'sudo hxb-net-autoconf forget';
		exec(command, cb);
	};

	this.complete_reset = function() {
		var command = nconf.get('debug-wizard') ? 'sleep 0' : 'sleep 5 && sudo reboot';
		exec(command, function() {});
	};

	this.registerMSG = function(cb) {
		var program = nconf.get('debug-wizard') ? '../backend/build/src/hexabus_msg_bridge --config bridge.conf' : 'sudo hexabus_msg_bridge';
		var register_command = nconf.get('debug-wizard') ? program + ' --create https://dev3-api.mysmartgrid.de:8443' : program + ' --create';
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
						nconf.save();
						cb({ step: 'register_code', error: undefined, code: stdout });
					}
				});
			}
		});
	};

	this.unregisterMSG = function(cb) {
		var script = nconf.get('debug-wizard') ? 'rm -f bridge.conf' : 'sudo rm -f /etc/hexabus_msg_bridge.conf; sudo svc -k /etc/service/hexabus_msg_bridge';
		exec(script, cb);
	};

	this.getActivationCode = function(cb) {
		var program = nconf.get('debug-wizard') ? '../backend/build/src/hexabus_msg_bridge --config bridge.conf' : 'sudo hexabus_msg_bridge';
		var retrieve_command = program + ' --activationcode';
		exec(retrieve_command, function(error, stdout, stderr) {
			if (error) {
				cb({ error: error });
			} else {
				cb({ error: undefined, code: stdout });
			}
		});
	};

	this.addDevice = function(devicetree, hexabus, cb) {
		var pairProgramm = nconf.get('debug-wizard') ? 'sleep 1' : 'hexapair -D /dev/ttyACM0 -t 30';
		
		var oldDevices = Object.keys(devicetree.devices);
		console.log(oldDevices);

		var newDevice;

		var doPairing = function(callback) {
			exec(pairProgramm, function(error, stdout, stderr) {
				if(error && stderr != 'Timeout, no device found.\n') {
					callback(stderr);
				}
				else {
					callback();
				}
			});
		};

		var enumerateNetwork = function(callback) {
			console.log('Reenumerating network');
			hexabus.update_devicetree(devicetree,callback);
		};

		var findNewDevice = function(callback) {
			for(var ip in devicetree.devices) {
				console.log('Device : ' + ip);
				if(oldDevices.indexOf(ip) == -1) {
					newDevice = devicetree.devices[ip];
				}
			}
			callback();
		};

		async.series([doPairing, enumerateNetwork, findNewDevice], function(error) {
			if(error !== undefined) {
				cb({'error': error});
				console.log(error);
			}
			else {
				if(newDevice !== undefined) {
					console.log('New Device: ' + newDevice.name);
					cb({'device': newDevice});
				}
				else {
					console.log("Error while querying device");
					cb({error: "Error while querying device"});
				}
			}
		});
	};
};

module.exports = Wizard;
