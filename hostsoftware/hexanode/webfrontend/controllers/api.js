'use strict';

module.exports.expressSetup = function(app, nconf, hexabus, devicetree) {
	app.get('/api', function(req, res) {
  		res.send("API is running.");
	});
	
	app.put('/api/sensor/:ip/:eid', function(req, res) {
		if (hexabus.is_ignored_endpoint(req.params.ip, req.params.eid)) {
			res.send("Ignored", 200);
			return;
		}
		var device = devicetree.devices[req.params.ip];
		if (device && device.endpoints[req.params.eid]) {
			res.send("Sensor exists", 200);
		} else {
			try {
				var endpoint = devicetree.add_endpoint(req.params.ip, req.params.eid, req.body);
				endpoint.last_value = parseFloat(req.body.value);
				res.send("Sensor added", 200);
			} catch(err) {
				console.log(err);
				console.log(err.stack);
				res.send(JSON.stringify(err), 400);
			}
		}
	});


	app.post('/api/sensor/:ip/:eid', function(req, res) {
		if (hexabus.is_ignored_endpoint(req.params.ip, req.params.eid)) {
			res.send("Ignored", 200);
			return;
		}
		var device = devicetree.devices[req.params.ip];
		if (!device || !device.endpoints[req.params.eid]) {
			res.send("Not found", 404);
		} else {
			var value = parseFloat(req.body.value);
			if (isNaN(value)) {
				res.send("Bad value", 400);
			} else {
				device.endpoints[req.params.eid].last_value = value;
				res.send("Value added");
			}
		}
	});


	app.post('/api/device/rename/:ip', function(req, res) {
		if (!req.body.name) {
			res.send("No name given", 400);
		} else {
			hexabus.rename_device(req.params.ip, req.body.name, function(err) {
				if (err) {
					res.send(JSON.stringify(err), 500);
				} else {
					res.send("Success");
					var device = devicetree.devices[req.params.ip];
					if (device) {
						device.name = req.body.name;
					}
				}
			});
		}
	});
};
