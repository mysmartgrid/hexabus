'use strict';

var exec = require('child_process').exec;
var v6 = require('ipv6').v6;
var os = require('os');
var fs = require('fs');
var nconf = require('nconf');
var async = require('async');
var ejs = require('ejs');

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

  var StatemachineBuilder = function() {
    this.ipToID = function(ip) {
      return ip.replace(/:/g,'_');
    }

    this.sm_folder = 'state_machines/';
    this.sm_build = this.sm_folder+'build/';
    this.targetFileList = [];
    this.fileContents = {};
    this.compileTarget = '';
    this.deviceList = [];
    this.progressCallback = Object;

    this.addTargetFile = function(src, target, context) {
      this.targetFileList.push({'src': src, 'target' : target, 'context' : context});
    }

    this.setCompileTarget = function(file) {
      this.compileTarget = file;
    }

    this.addDevice = function(devicename) {
      this.deviceList.push(devicename);
    }

    this.onProgress = function(callback) {
      this.progressCallback = callback;
    }

    this.setProgress = function(msg, localization, extras) {
		extras = extras || {};
		this.progressCallback({ 'msg' : msg, 'localization' : localization, 'extras' : extras});
    }

	this.localizeError = function(localization, error, extras) {
		extras = extras || {};
		return {'msg': error.toString(), 'localization': localization, 'extras' : extras};
	}

	this.localizeErrorCallback = function (localization, extras, callback) {
		var localizedErrorCallback = function(err) {
			if(err) {
				callback(this.localizeError(localization, err, extras));
			}
			else {
				callback();
			}
		}
		return localizedErrorCallback.bind(this);
	}


    this.readFiles = function(callback) {
      var readFile = function(file,callback) {
        if(!(file.src in this.fileContents)) {
          console.log('Reading File: ' + file.src);
          fs.readFile(this.sm_folder + file.src,  { encoding: 'utf8' }, function(err, data) {
            if(err) {
				console.log(err);
				callback(this.localizeError('opening-template-failed',err,{'file': file.src}));
            }
            else {
              this.fileContents[file.src] = data;
              callback();
            }}.bind(this));
        }
        else {
          console.log('File ' + file.src + ' is read already.');
          callback();
        }
      }

      console.log('Reading template files');
      this.setProgress('Reading files', 'reading');
      async.eachSeries(this.targetFileList,readFile.bind(this),callback);
    }


    this.renderTemplates = function(callback) {
      var renderTemplate = function(file, callback) {
        console.log('Rendering template ' + file.src + ' to ' + file.target);
        var renderedTemplate = ejs.render(this.fileContents[file.src], file.context);
        fs.writeFile(this.sm_build + file.target, renderedTemplate, { encoding: 'utf8' }, function(err) {
			if(err) {
				console.log(err);
				callback('writing-file-failed');
			}
			else {
				callback();
			}
		});
      }

      this.setProgress('Rendering templates', 'rendering');
      async.each(this.targetFileList,renderTemplate.bind(this),callback);
    }

	this.compileStatmachines = function(callback) {
		console.log('Compiling statemachine');
		this.setProgress('Compiling statemachine', 'compiling');
		exec('hbcomp ' + this.sm_build +  this.compileTarget + ' -o ' + this.sm_build + ' -d ' + this.sm_folder + 'datatypes.hb', function(err, stdout, stderr) {
			if(err) {
				console.log(err);
				callback('compiling-failed');
			}
			else {
				callback();
			}
		});
    }

    this.assembleStatemachines = function(callback) {
      var assembleStatemachine = function(device, callback) {
        console.log('Assembling statemachine ' + device);
        exec('hbasm ' + this.sm_build + device + '.hba' + ' -d ' + this.sm_folder + 'datatypes.hb -o ' + this.sm_build + device + '.hbs', function(err, stdout, stderr) {
			if(err) {
				console.log(err);
				callback('assembling-failed');
			}
			else {
				callback();
			}
		});
      }

      this.setProgress('Assembling statemachines', 'assembling');
      async.each(this.deviceList, assembleStatemachine.bind(this), callback);
    }


    this.uploadStatemachines = function(callback) {
      var deviceCounter = 0;

      var uploadStatemachine = function(device, callback) {
        console.log('Uploading to ' + device);
        deviceCounter += 1;
        this.setProgress('Uploading statemachine ' + device, 'uploading', { 'device' : device, 'done' : deviceCounter, 'count' : this.deviceList.length});
        exec('hexaupload -r 10 -k -p ' + this.sm_build + device + '.hbs', function(err, stdout, stderr) {
			if(err) {
				console.log(err);
				callback('uploading-failed');
			}
			else {
				callback();
			}
		});
      }

      async.eachSeries(this.deviceList, uploadStatemachine.bind(this), callback);
    }

	this.cleanUp = function(callback) {
      var deleteFile = function(file, callback) {
        console.log('Deleting temporary file: ' + file);
        fs.unlink(this.sm_build + file, function(err) {
			if(err) {
				console.log(err);
				callback('deleting-temporary-files-failed');
			}
			else {
				callback();
			}
		});
      }

      var fileList = [];
      for(var file in this.targetFileList) {
        fileList.push(this.targetFileList[file].target);
      }
      for(var device in this.deviceList) {
        fileList.push(this.deviceList[device] + '.hba');
        fileList.push(this.deviceList[device] + '.hbs');
      }

      this.setProgress('Deleting temporary files', 'deleting-temporary-files');
      async.each(fileList, deleteFile.bind(this), callback);
    }

    this.buildStatemachine = function(callback) {
      console.log('Building statemachine');
      async.series([this.readFiles.bind(this),
                    this.renderTemplates.bind(this),
                    this.compileStatmachines.bind(this),
                    this.assembleStatemachines.bind(this),
                    this.uploadStatemachines.bind(this),
                    this.cleanUp.bind(this)],callback);
    }
  }


  this.master_slave_sm = function(msg, progressCallback, callback) {
   var smb = new StatemachineBuilder();

   smb.onProgress(progressCallback);

   smb.addTargetFile('master.hbh', 'master.hbh', { 'masterip' : msg.master.ip});
   smb.addDevice('master');

   var slavelist = []
   for(var slave in msg.slaves) {
    var name = 'slave_' + smb.ipToID(msg.slaves[slave].ip);
    console.log(name);
    smb.addTargetFile('slave.hbh', name + '.hbh', {'slavename' : name, 'slaveip' : msg.slaves[slave].ip});
    smb.addDevice(name);
    slavelist.push(name);
  }


  smb.addTargetFile('master_slave.hbc', 'master_slave.hbc', {'threshold' : msg.threshold, 'slaves' : slavelist});
  smb.setCompileTarget('master_slave.hbc');

  smb.buildStatemachine(function(err) {
    if(!err) {
      callback(true);
    } else {
      callback(false, err);
    }});
  }


  this.standbykiller_sm = function(msg, progressCallback, callback) {
    var smb = new StatemachineBuilder();

    console.log('Building standbykiller');

    smb.onProgress(progressCallback);

    smb.addTargetFile('standbykiller.hbh', 'standbykiller.hbh', { 'ip' : msg.device.ip});
    smb.addTargetFile('standbykiller.hbc', 'standbykiller.hbc', {'threshold' : msg.threshold, 'timeout' : msg.timeout});
    smb.addDevice('standbykiller');

    smb.setCompileTarget('standbykiller.hbc');

    smb.buildStatemachine(function(err) {
      if(!err) {
        callback(true);
      } else {
        console.log(err);
        callback(false, err.toString());
      }});
  }
}

module.exports = hexabus;
