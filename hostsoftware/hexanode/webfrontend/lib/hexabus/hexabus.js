var exec = require('child_process').exec;
var v6 = require('ipv6').v6;
var fs = require('fs');

var hexabus = function() {
	this.rename_device = function(addr, newName, cb) {
		addr = new v6.Address(addr).canonicalForm();
		var file = "/tmp/name_" + addr + ".hba";
		var assembly = file + ".hbs";
		var program = "target " + addr + ";\n" +
			"device_name " + newName + ";\n" + 
			"machine 0;";
		fs.writeFile(file, program, function(err) {
			if (err) {
				throw err;
			} else {
				exec("hbasm -i " + file + " -o " + assembly + " -d /usr/share/hexabus/std_datatypes.hb", function(error, stdout, stdderr) {
					if (err) {
						throw err;
					} else {
						exec("hexaupload -p " + assembly + " && rm " + assembly, function(err, stdout, stderr) {
							console.log(stdout);
							if (cb)
								cb();
						});
					}
				});
			}
		});
	};
};

module.exports = hexabus;
