angular.module('hexanode')
.controller('devicesList', ['$scope', '$timeout', 'Socket', 'HexabusClient', 'Lang', function($scope, $timeout, Socket, hexabusclient, Lang) {
	$scope.Lang = Lang;

	$scope.devicetree = new window.DeviceTree();
	
	$scope.show_rename = function(device) {
		device.rename = true;
	};

	$scope.rename_device = function(device) {
		device.renaming = true;
		hexabusclient.renameDevice(device, device.new_name, function(data) {
			if(!data.success) {
				console.log('Renaming failed !');
				console.log(data.error);
				device.rename_error_code = data.error;
				device.rename_error = true;
			}
			else {
				device.rename = false;
			}
			device.renaming = false;
		});
	};

	var updateDisplay = function() {
		//Calling $timeout is sufficient to trigger an update of the angular bindings
		$timeout(updateDisplay, 1000);
	};


	$scope.remove = function(device) {
		var message = "Do you really want to remove {device}?";

		message = ((Lang.pack["wizard"] || {})["device-list"] || {})["forget-dialog"] || message;

		message = message.replace("{device}", device.name);
		if (confirm(message)) {
			hexabusclient.removeDevice(device);
		}
	};

	Socket.on('devicetree_init', function(json) {
		$scope.devicetree = new window.DeviceTree(json);

		$scope.devicetree.on('update', function(update) {
			Socket.emit('devicetree_update', update);
		});

		$scope.devicetree.on('delete', function(deletion) {
			Socket.emit('devicetree_delete', deletion);
		});
	});

	Socket.on('devicetree_update', function(update) {
		$scope.devicetree.applyUpdate(update);
	});

	Socket.on('devicetree_delete', function(deletion) {
		$scope.devicetree.applyDeletion(deletion);
	});

	Socket.emit('devicetree_request_init');

	hexabusclient.enumerateNetwork(function(data) {
		//TODO: handle hexinfo errors
		$scope.scanDone = true;
		console.log('Scan done');		
	});

	updateDisplay();

}])
.controller('devicesAdd', ['$scope', 'Socket', 'Lang', function($scope, Socket, Lang) {
	
	$scope.search = function() {
		Socket.emit('devices_add');
		$scope.error = false;
		$scope.found = false;
		$scope.rename = false;
		$scope.searching = true;
	};

	var on_device_found = function(msg) {
		$scope.searching = false;
		if(msg.error !== undefined) {
			$scope.error = true;
			$scope.errormsg = msg.error;
		} else {
			var eids = {};
			for(var key in msg.device.endpoints) {
				if(msg.device.endpoints[key].function != "infrastructure") {
					eids[key]=msg.device.endpoints[key];
				}
			}
			msg.device.endpoints = eids;
			$scope.device = msg.device;
			$scope.new_name = msg.device.name;
			$scope.found = true;
		}
	};

	var on_ep_metadata = function(ep) {
		if($scope.renaming && ep.ip == $scope.device.ip) {
			if($scope.new_name == ep.name) {
				$scope.device.name = ep.name;
				$scope.renaming = false;
				$scope.rename = false;
				$scope.rename_error_class = "";
				$scope.rename_error = false;
			}
		}
	};

	var on_device_rename_error = function(msg) {
		if($scope.renaming) {
			$scope.renaming = false;
			$scope.rename_error_msg = msg.error.code || msg.error;
			$scope.rename_error_class = "error";
			$scope.rename_error = true;
		}
	};

	$scope.show_rename = function() {
		$scope.rename = true;
	};

	$scope.rename_device = function(ip) {
		$scope.renaming = true;
		var name = $scope.new_name;
		Socket.emit('device_rename', {device: ip, name: name});
	};

	Socket.on('device_found', on_device_found);
	Socket.on('ep_metadata', on_ep_metadata);
	Socket.on('device_rename_error', on_device_rename_error);
}]);
