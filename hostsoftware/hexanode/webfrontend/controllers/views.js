'use strict';
var View = require('../lib/devicetree').View;

/*
 * Controller for creating, updating an deleting devicetree view.
 *
 * Devicetree views provide a basic mechanism for allowing users to group endpoints together for display.
 * On the browserside devicetree views are displayed as different tabs on the overview page.
 */


module.exports.expressSetup = function(app, nconf, hexabus, devicetree) {

	/*
	 * Create new devicetree view.
	 *
	 * The new devicetree view is always called "Unnamed View".
	 * The browser will be redirected to the edit page for the freshly created devicetree view.
	 */
	app.get('/view/new', function(req, res) {
		var count = Object.keys(devicetree.views).length; 

		var view = devicetree.addView("Unnamed View", {});

		res.redirect('/view/edit/' + view.id);
	});


	/*
	 * Display the edit page for a devicetree view.
	 */
	app.get('/view/edit/:id', function(req, res) {
		var view = devicetree.views[req.params.id];

		if (!view) {
			res.send("view " + req.params.id + " not found", 404);
			return;
		}

		res.render('view/edit.ejs', {
			active_nav: 'configuration',
			view_name: view.name,
			view_id: req.params.id
		});
	});


	/*
	 * Update the enpoint list for a devicetree view.
	 *
	 * The endpoint list has be provided as json inside the
	 * post parameter named 'endpoint_order'.
	 */
	app.post('/view/edit/:id', function(req, res) {
		var view = devicetree.views[req.params.id];

		if (!view) {
			res.send("view not found", 404);
			return;
		}

		view.endpoints = JSON.parse(req.body.endpoint_order);
		view.name = req.body.view_name;

		res.redirect('/');
	});


	/*
	 * Deletes a devicetree view.
	 */
	app.get('/view/delete/:id', function(req, res) {

		if (!devicetree.views[req.params.id]) {
			res.send("view " + req.params.id + " not found", 404);
			return;
		}
		
		devicetree.removeView(req.params.id);

		res.redirect('/');
	});
};
