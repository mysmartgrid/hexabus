'use strict';
var View = require('../lib/devicetree').View;

module.exports.expressSetup = function(app, nconf, hexabus, devicetree) {

	app.get('/view/new', function(req, res) {
		var count = Object.keys(devicetree.views).length; 

		var view = devicetree.addView("Unnamed View", {});

		res.redirect('/view/edit/' + view.id);
	});

	app.get('/view/edit/:id', function(req, res) {
		var view = devicetree.views[req.params.id];

		if (!view) {
			res.send("view " + req.params.id + " not found", 404);
			return;
		}

		var devices = {};
		var used = view.endpoints;

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
			view_name: view.name,
			view_id: req.params.id
		});
	});

	app.post('/view/edit/:id', function(req, res) {
		var view = devicetree.views[req.params.id];

		if (!view) {
			res.send("view not found", 404);
			return;
		}

		console.log(req.body.endpoint_order);

		view.endpoints = JSON.parse(req.body.endpoint_order);
		view.name = req.body.view_name;

		res.redirect('/');
	});


	app.get('/view/delete/:id', function(req, res) {

		if (!devicetree.views[req.params.id]) {
			res.send("view " + req.params.id + " not found", 404);
			return;
		}
		
		devicetree.removeView(req.params.id);

		res.redirect('/');
	});
};
