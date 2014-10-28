'use strict';

/*
 * A Serverobject that exposes some hexabus functions via socket io.
 *
 * The browser side code uses the functions exposed by this object to
 * perform various interaction with hexbus devices.
 */


var Hexabus = require("../lib/hexabus");

var HexabusServer = function(socket, devicetree) {

	var hexabus = new Hexabus();

	// Wrapper for socket.on with improved exception handling.
	var on = function(ev, cb) {
		socket.on(ev, function(data, ackcb) {
			try {
				cb(data, ackcb);
			} catch (e) {
				socket.emit('_hexabus_error_', e);
				console.log('HexabusServer error: ' + e);
			}
		});
	};


	/*
	 * Update the metadata of endpoint in the devicetree.
	 *
	 * Expected format for the incoming socket.io message:
	 * {
	 *	 enpointId: "<deviceIp>.<eid>",
	 *	 field: "<NameOfTheMetadataField>",
	 *	 value: "<NewValueForTheMetadataField>"
	 * }
	 */
	on('hexabus_update_endpoint_metadata', function(data) {
		if(data.endpointId === undefined || data.field === undefined || data.value === undefined) {
			throw 'Invalid enpoint metadata update';
		}
		
		var ep = devicetree.endpoint_by_id(data.endpointId);
		if(ep === undefined) {
			throw 'Unknown endpoint';
		}

		if(ep[data.field] === undefined) {
			throw 'Unknown field';
		}

		ep[data.field] = data.value;
	});


	/*
	 * Rename a hexabus device.
	 *
	 * Expected format for the incoming socket.io message:
	 * {
	 *	 deviceIp: "<IpOfTheDevice>",
	 *	 name: "<NewNameForTheDevice>"
	 * }
	 */
	on('hexabus_rename_device', function(data, cb) {
		if(data.deviceIp === undefined || data.name === undefined) {
			throw 'Invalid rename';
		}


		hexabus.rename_device(data.deviceIp, data.name, function(error) {
			if(error) {
				cb({'success' : false, 'errorCode' : error.code});
			}
			else {
				devicetree.devices[data.deviceIp].name = data.name;
				cb({'success' : true});
			}
		});
	});

	
	/*
	 * Write a new value to an endpoint.
	 *
	 * Can for example be used to switch a relais off and on.
	 *
	 * Expected format for the incoming socket.io message:
	 * {
	 *	 endpointId: "<deviceIp>.<eid>",
	 *	 value: "<NewValueForTheEndpoint>"
	 * }
	 */
	on('hexabus_set_endpoint', function(data, cb) {
		
		if(data.endpointId === undefined || data.value === undefined) {
			throw 'Invalid set endpoint';
		}

		var ep = devicetree.endpoint_by_id(data.endpointId);
		if(ep === undefined) {
			throw 'Unknown endpoint';
		}

		hexabus.write_endpoint(ep.ip, ep.eid, ep.type, data.value, function(error) {
			console.log(error);
			if(error) {
				cb({'success' : false, 'error' : error.toString()});
			}
			else {
				ep.last_value = data.value;
				cb({'success' : true});
			}
		});
	});

	/*
	 * Enumerate the hexabus network.
	 *
	 * Runs device discovery in the hexabus network and adds newly found device to
	 * the devicetree.
	 */
	on('hexabus_enumerate', function(data, cb) {
		console.log('Enumerate called');
		hexabus.update_devicetree(devicetree, function(error) {
			if (error === undefined) {
				cb({'success' : true});
			}
			else {
				console.log(error);
				cb({'success' : false, 'msg' : error});
			}
		});
	});


};


module.exports = HexabusServer;
