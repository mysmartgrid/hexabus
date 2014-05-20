'use strict';

var async = require('async');
var ejs = require('ejs');
var exec = require('child_process').exec;
var fs = require('fs');


exports.validationError = function(localization, error, extras) {
	this.msg = error.toString();
	this.localization = localization;
	this.extras = extras || {};
}


exports.StatemachineBuilder = function() {
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


	this.setupBuildDir = function(callback) {
		fs.mkdir(this.sm_build,function(err) {
			if(err && err.code != 'EEXIST') {
				callback(this.localizeError('creatin-builddir-failed',err));
			}
			else {
				callback();
			}
		});
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
		async.series([this.setupBuildDir.bind(this),
					this.readFiles.bind(this),
					this.renderTemplates.bind(this),
					this.compileStatmachines.bind(this),
					this.assembleStatemachines.bind(this),
					this.uploadStatemachines.bind(this),
					this.cleanUp.bind(this)],callback);
	}
}
