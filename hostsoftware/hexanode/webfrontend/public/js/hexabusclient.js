window.HexabusClient = function(socket) {
	this.updateEndpointMetadata = function(endpoint, field, value) {
		socket.emit('hexabus_update_endpoint_metadata', {'endpointId' : endpoint.id, 'field' : field, 'value' : value});
	};

	this.renameDevice = function(device, name, cb) {
		socket.emit('hexabus_rename_device', {'deviceIp' : device.ip, 'name' : name}, cb);
	};

	this.setEndpoint = function(endpoint, value, cb) {
		socket.emit('hexabus_set_endpoint', {'endpointId' : endpoint.id, 'value' : value}, cb);
	};

	this.enumerateNetwork = function(cb) {
		console.log('Enumerating network');
		socket.emit('hexabus_enumerate', {}, cb);
	};

};
