'use strict';

module.exports.expressSetup = function(app, nconf, hexabus, devicetree) {
	app.get('/statemachine', function(req, res) {
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

		res.render('statemachine/statemachine.ejs', { active_nav: 'statemachines', devices: devices });
	});
};
