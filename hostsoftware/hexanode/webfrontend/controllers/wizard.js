'use strict';

var Wizard = require("../lib/wizard");

module.exports.expressSetup = function(app, nconf, hexabus, devicetree) {
	app.get('/wizard', function(req, res) {
			var wizard = new Wizard();
			if (wizard.is_finished()) {
				res.redirect('/wizard/current');
			} else {
				res.redirect('/wizard/new');
			}
	});

	app.get('/wizard/current', function(req, res) {
		var config = hexabus.read_current_config();
		hexabus.get_activation_code(function(err, activation_code) {
			config.activation_code = activation_code;

			hexabus.get_heartbeat_state(function(err, state) {
				config.active_nav = 'configuration';

				if (err) {
					config.heartbeat_ok = false;
					config.heartbeat_code = 0;
					config.heartbeat_messages = [err];
					config.heartbeat_state = "";
				} else {
					config.heartbeat_ok = state.code === 0;
					config.heartbeat_code = state.code;
					config.heartbeat_messages = state.messages;
					config.heartbeat_state = state;
				}

				res.render('wizard/current.ejs', config);
			});
		});
	});

	app.post('/wizard/reset', function(req, res) {
		devicetree.reset();

		var wizard = new Wizard();
		wizard.deconfigure_network(function(err) {
			if (err) {
				res.send(JSON.stringify({ step: "deconfigure_network", error: err }), 500);
				return;
			}
			wizard.unregisterMSG(function(err) {
				if (err) {
					res.send(JSON.stringify({ step: "unregisterMSG", error: err }), 500);
					return;
				}
				res.redirect('/wizard/resetdone');
			});
		});
	});

	app.get('/wizard/resetdone', function(req, res) {
		res.render('wizard/resetdone.ejs', { active_nav: 'configuration' });
		nconf.set('wizard_step', undefined);
		nconf.save();
		var wizard = new Wizard();
		wizard.complete_reset();
	});

	app.get('/wizard/devices', function(req, res) {
		var devices = {};

		devicetree.forEach(function(device) {
			var entry = devices[device.ip] = { name: device.name, ip: device.ip, eids: [] };

			device.forEachEndpoint(function(ep) {
				if (ep.function != "infrastructure") {
					entry.eids.push(ep);
				}
			});
			entry.eids.sort(function(a, b) {
				return b.eid - a.eid;
			});
		});

		res.render('wizard/devices.ejs', { active_nav: 'configuration', devices: devices });
	});

	app.get('/wizard/devices/add', function(req, res) {
		res.render('wizard/add_device.ejs', { active_nav: 'configuration' });
	});

	app.get('/wizard/:step', function(req, res) {
		// steps 1-... and 0 if completed, undefined if not started
		var wizard_step = nconf.get('wizard_step');
		var error = false;
		var getCurrentStep = function(wizard_step) {
			switch(wizard_step) {
				case undefined: return 'new';
				case '0': return 'reset';
				case '1': return 'connection';
				case '2': return 'activation';
				case '3': return 'msg';
				default: return 'reset';
			}
		};

		var current_step = getCurrentStep(wizard_step);

		switch(req.params.step) {
			case "new":
				if(wizard_step !== undefined) {
					res.render('wizard/' + current_step  + '.ejs', { active_nav: 'configuration', error: 'gt_error' });
				} else {
					nconf.set('wizard_step', '1');
					nconf.save();
					res.render('wizard/new.ejs', { active_nav: 'configuration' });
				}
				return;
			case "connection":
				if(wizard_step === undefined) {
					nconf.set('wizard_step', '1');
					nconf.save();
					res.render('wizard/new.ejs', { active_nav: 'configuration', error: 'lt_error' });
				} else if(wizard_step>'1' || wizard_step=='0') {
					res.render('wizard/' + current_step  + '.ejs', { active_nav: 'configuration', error: 'gt_error' });
				} else {
					
					res.render('wizard/' + req.params.step  + '.ejs', { active_nav: 'configuration' });
				}
				return;
			case "activation":
				if(wizard_step === undefined) {
					nconf.set('wizard_step', '1');
					nconf.save();
					res.render('wizard/new.ejs', { active_nav: 'configuration', error: 'lt_error' });
				} else if(wizard_step<'2') {
					res.render('wizard/' + current_step  + '.ejs', { active_nav: 'configuration', error: 'lt_error' });
				} else if(wizard_step>'2' || wizard_step=='0') {
					res.render('wizard/' + current_step  + '.ejs', { active_nav: 'configuration', error: 'gt_error' });
				} else {
					res.render('wizard/' + req.params.step  + '.ejs', { active_nav: 'configuration' });
				}
				return;
			case "msg":
				if(wizard_step === undefined) {
					nconf.set('wizard_step', '1');
					nconf.save();
					res.render('wizard/new.ejs', { active_nav: 'configuration', error: 'lt_error' });
				} else if(wizard_step<'3') {
					res.render('wizard/' + current_step  + '.ejs', { active_nav: 'configuration', error: 'lt_error' });
				} else if(wizard_step>'3' || wizard_step=='0') {
					res.render('wizard/' + current_step  + '.ejs', { active_nav: 'configuration', error: 'gt_error' });
				} else {
					res.render('wizard/' + req.params.step  + '.ejs', { active_nav: 'configuration' });
				}
				return;
			case "success":
				if(wizard_step === undefined) {
					nconf.set('wizard_step', '1');
					nconf.save();
					res.render('wizard/new.ejs', { active_nav: 'configuration', error: 'lt_error' });
				} else if(wizard_step<'3') {
					res.render('wizard/' + current_step  + '.ejs', { active_nav: 'configuration', error: 'lt_error' });
				} else if(wizard_step>'3' || wizard_step=='0') { // reset or display success page?
					res.render('wizard/' + current_step  + '.ejs', { active_nav: 'configuration', error: 'gt_error' });
				} else {
					nconf.set('wizard_step', '0');
					nconf.save();
					res.render('wizard/' + req.params.step  + '.ejs', { active_nav: 'configuration' });
				}
				return;
		}
		res.render('wizard/' + req.params.step  + '.ejs', { active_nav: 'configuration' });
	});

};

module.exports.socketioSetup = function(on, emit, devicetree) {
	
	on('upgrade', function() {
		var wizard = new Wizard();

		wizard.upgrade(function(error) {
			emit('upgrade_completed', { msg: error });
		});
	});

	on('wizard_configure', function() {
		var wizard = new Wizard();

		wizard.configure_network(function(progress) {
			emit('wizard_configure_step', progress);
		});
	});

	on('wizard_register', function() {
		var wizard = new Wizard();

		wizard.registerMSG(function(progress) {
			emit('wizard_register_step', progress);
		});
	});

	on('get_activation_code', function() {
		var wizard = new Wizard();

		wizard.getActivationCode(function(data) {
			emit('activation_code', data);
		});
	});

	on('devices_add', function() {
		var wizard = new Wizard();

		wizard.addDevice(devicetree, function(msg) {
			emit('device_found', msg);
		});
	});
};
