'use strict';

var Hexabus = require("../lib/hexabus");

var HexabusServer = function(socket, devicetree) {
	
	var hexabus = new Hexabus();

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

	on('hexabus_enumerate', function(data, cb) {
		console.log('Enumerate called');
		hexabus.enumerate_network(function(dev) {
			if (dev.done) {
				cb({'success' : true});
			} else if (dev.error) {
				console.log(dev.error);
				cb({'success' : false, 'msg' : dev.error});
			} else {
				dev = dev.device;
				for (var key in dev.endpoints) {
					var ep = dev.endpoints[key];
					ep.name = ep.name || dev.name;
					if ((!devicetree.devices[dev.ip] || !devicetree.devices[dev.ip].endpoints[ep.eid]) && !hexabus.is_ignored_endpoint(dev.ip, ep.eid)) {
						devicetree.add_endpoint(dev.ip, ep.eid, ep);
					}
					else if(devicetree.devices[dev.ip] && devicetree.devices[dev.ip].endpoints[ep.eid] && !hexabus.is_ignored_endpoint(dev.ip, ep.eid)) {
						devicetree.devices[dev.ip].endpoints[ep.eid].update();
					}
				}
			}
		});
	});


};


module.exports = HexabusServer;
