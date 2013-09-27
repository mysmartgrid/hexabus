var exec = require('child_process').exec;
var v6 = require('ipv6').v6;

var hexabus = function() {
	this.rename_device = function(addr, newName, cb) {
		addr = new v6.Address(addr).canonicalForm();
		exec("hexaupload --ip " + addr + " --rename '" + newName.replace("'", "''") + "'", function(error, stdout, stderr) {
			if (error) {
				throw error;
			} else if (cb) {
				cb();
			}
		});
	};
};

module.exports = hexabus;
