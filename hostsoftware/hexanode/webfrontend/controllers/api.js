'use strict';

/*
 * Controller for the REST API
 *
 * The REST API is used by the hexanode backend to push new values
 * from hexabus packets into the node application.
 */


module.exports.expressSetup = function(app, nconf, hexabus, devicetree) {
	// Simple status page used by the backend to check if the frontend is running.
	app.get('/api', function(req, res) {
  		res.send("API is running.");
	});
	
	/*
	 * Add a new endpoint or device.
	 *
	 * If a put request is send to /api/sensor/:ip/:eid and there is no device with this ip,
	 * a new device with a new endpoint for the eid will be created.
	 * If there is allready a device for this ip but it does not have a endpoint for this eid,
	 * a new endpoint with this eid is created for the device.
	 * If a device with this ip, which also has an endpoint for this eid, exist already nothing is done.
	 * If the eid is on the list of ignored eids (== hexabus.is_ignored_endpoint returns true)
	 * the request will be ignored.
	 */
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

	/*
	 * Update the value of an endpoint.
	 *
	 * A post request to /api/sensor/:ip/:eid can be used to update the value a devices endpoint.
	 */
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


	// Legacy code
	//TODO/Homework: Check whether this is used anywhere at all
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
