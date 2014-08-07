var validator = require('validator');
var common = require('./common');
var validationError = common.validationError;
var StatemachineBuilder = common.StatemachineBuilder;


exports.validateInput = function(msg) {
	
	if(!msg.hasOwnProperty('times') || !(msg.times instanceof Array) || msg.times.length <= 0) {
		return new validationError('times-invalid', 'No switching times given.');
	}

	if(!msg.hasOwnProperty('device') || !msg.device.hasOwnProperty('ip') || !validator.isIP(msg.device.ip,6)) {
		return new validationError('device-ip-not-valid', 'The IPv6 address of device is invalid.');
	}
	
}


exports.buildMachine = function(msg, progressCallback, callback) {
	var smb = new StatemachineBuilder();

	console.log('Building timetable switch');

	smb.onProgress(progressCallback);

	var dateSort = function(a,b) {
		return (a.hour<=b.hour&&a.minute<=b.minute)?((a.hour<b.hour||a.minute<b.minute)?-1:0):1;
	}
	msg.times.sort(dateSort);

	var tH = -1;
	var iH = -1;
	var iM = 0;
	var times = [];
	for(var h in msg.times) {
		if(msg.times[h].hour>tH) {
			tH = msg.times[h].hour;
			iH++;
			times[iH] = {hour:msg.times[h].hour, minutes: []}
		}

		times[iH].minutes.push({minute: msg.times[h].minute, on: msg.times[h].on});
		iM++;
	}

	smb.addTargetFile('timetable/plug.hbh', 'plug.hbh', { 'ip' : msg.device.ip});
	smb.addTargetFile('timetable/timetable_hm.hbc', 'timetable_hm.hbc', {'times' : times});
	smb.addDevice('plug',true);

	smb.setCompileTarget('timetable_hm.hbc');

	smb.buildStatemachine(function(err) {
		if(!err) {
			callback(true);
		} else {
			console.log(err);
			callback(false, err);
	}});
}
