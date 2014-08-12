'use strict';


module.exports.expressSetup = function(app, nconf, hexabus, devicetree) {

	app.get('/view/new', function(req, res) {
		var count = Object.keys(devicetree.views).length; 

		var id = Math.round(Date.now() / 1000) + "." + count;

		devicetree.views[id] = {
			id: id,
			name: "Unnamed View",
			devices: []
		};

		res.redirect('/view/edit/' + id);
	});

	app.get('/view/edit/:id', function(req, res) {
		var view = devicetree.views[req.params.id];

		if (!view) {
			res.send("view " + req.params.id + " not found", 404);
			return;
		}

		var devices = {};
		var used = view.devices;

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

		res.render('view/edit.ejs', {
			active_nav: 'configuration',
			known_devices: devices,
			used_devices: used,
			view_name: view.name,
			view_id: req.params.id
		});
	});

	app.get('/view/delete/:id', function(req, res) {

		if (!devicetree.views[req.params.id]) {
			res.send("view " + req.params.id + " not found", 404);
			return;
		}
		
		delete devicetree.views[req.params.id];
		
		var devicetree_file = nconf.get('config') + '/devicetree.json';
		devicetree.save(devicetree_file);

		res.redirect('/');
	});


	app.post('/view/edit/:id', function(req, res) {
		var view = devicetree.views[req.params.id];

		if (!view) {
			res.send("view not found", 404);
			return;
		}

		view.devices = JSON.parse(req.body.device_order);
		view.name = req.body.view_name;

		var devicetree_file = nconf.get('config') + '/devicetree.json';
		devicetree.save(devicetree_file);

		res.redirect('/');
	});

}
