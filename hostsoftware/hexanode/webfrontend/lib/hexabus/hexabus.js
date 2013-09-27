var exec = require('child_process').exec;
var v6 = require('ipv6').v6;

var hexabus = function() {
	this.rename_device = function(addr, newName, cb) {
		addr = new v6.Address(addr);
		if (!addr.valid) {
			throw "Invalid address";
		}
		var command = "hexaupload --ip " + addr.canonicalForm() + " --rename '" + newName.replace("'", "''") + "'";
		exec(command, function(error, stdout, stderr) {
			if (error && cb) {
				cb(error);
			} else if (cb) {
				cb();
			}
		});
	};
};

module.exports = new hexabus();
