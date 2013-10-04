var nconf = require('nconf');
var exec = require('child_process').exec;

var Wizard = function() {
	var networkAutoconf = function(cb) {
		var command = nconf.get('debug_wizard')
			? 'sleep 2'
			: 'sudo hxb-net-autoconf';
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

	this.registerMsg = function(cb) {
		var program = nconf.get('debug_wizard')
			? '../backend/build/src/hexabus_msg_bridge --config bridge.conf'
			: 'sudo hexabus_msg_bridge';
		var register_command = nconf.get('debug_wizard')
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
};

module.exports = Wizard;
