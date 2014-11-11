angular.module('hexanode')
.controller('dashboardController', ['$scope', 'Socket', 'HexabusClient', '$timeout', 'Lang', function($scope, Socket, hexabusclient, $timeout, Lang) {

	$scope.devicetree = new window.DeviceTree();
	$scope.endpointById = $scope.devicetree.endpoint_by_id.bind($scope.devicetree);

	var lastTreeUpdateReceivedAt;

	$scope.lastUpdate = "never";
	$scope.errorMessage = "";

	$scope.current_view = {};
	$scope.hide_unitless = true;

	var getEndpointsByFunction = function(fun) {
		var endpoints = [];

		$scope.devicetree.forEach(function(device) {
			device.forEachEndpoint(function(endpoint) {
				if(endpoint.function == fun) {
					endpoints.push(endpoint.id);
				}
			});
		});

		return endpoints;
	};

	$scope.setView = function(view_id) {
		if (view_id == "sensors") {
			$scope.current_view = {
				id: "sensors",
				name: Lang.pack["dashboard"]["all-sensors"],
				endpoints: getEndpointsByFunction('sensor')
			};
			$scope.hide_unitless = true;
		} else if (view_id == "actors") {
			$scope.current_view = {
				id: "actors",
				name: Lang.pack["dashboard"]["all-actors"],
				endpoints: getEndpointsByFunction('actor')
			};
			$scope.hide_unitless = false;
		} else {
			$scope.current_view = $scope.devicetree.views[view_id];
			$scope.hide_unitless = false;
		}
		console.log('SetView: ' + view_id);
	};


	var pendingUpdateControl = null;

	$scope.editBegin = function(endpoint, data) {
		pendingUpdateControl = $scope.devicetree.endpoint_by_id(endpoint.id).control;
	};

	var sensorEditDone = function(endpoint, data) {
		var entry = $scope.devicetree.endpoint_by_id(endpoint.id);
		if (data.minvalue !== undefined) {
			hexabusclient.updateEndpointMetadata(entry, 'minvalue', data.minvalue);
		} else if (data.maxvalue !== undefined) {
			hexabusclient.updateEndpointMetadata(entry, 'maxvalue', data.maxvalue);
		} else if (data.name !== undefined) {
			entry.control.coverClass();
			hexabusclient.renameDevice(entry.device, data.name, function(data) {
				console.log(data);
				if(!data.success) {
					$scope.errorMessage = Lang.pack["rename-error"]["template"].replace("{reason}", Lang.pack["rename-error"]["timeout"]);
				}
				entry.control.uncover();
			});
		}
		pendingUpdateControl = null;
		updateDisplay();
	};


	var actorEditDone = function(endpoint, data) {
		hexabusclient.setEndpoint(endpoint, data.value ? 1 : 0, function(data) {
			if(!data.success) {
				$scope.errorMessage = 'Error setting endpoint: ' + data.error;
			}
		});
	};

	$scope.editDone = function(endpoint, data) {
		if (endpoint.function == "sensor") {
			sensorEditDone(endpoint, data);
		} else if (endpoint.function == "actor") {
			actorEditDone(endpoint, data);
		}
	};

	var updateDisplayScheduled;
	var updateDisplay = function() {
		if (updateDisplayScheduled) {
			$timeout.cancel(updateDisplayScheduled);
		}

		if (lastTreeUpdateReceivedAt) {
			var secondsDiff = Math.round((Date.now() - lastTreeUpdateReceivedAt) / 1000);
			$('#last-update-when').text(Lang.localizeLastUpdate(lastTreeUpdateReceivedAt / 1000));
		}


		if (pendingUpdateControl) {
			pendingUpdateControl.focus();
		}

		updateDisplayScheduled = $timeout(updateDisplay, 1000);
	};

	Socket.on('devicetree_init', function(json) {
		$scope.devicetree = new window.DeviceTree(json);
		$scope.endpointById = $scope.devicetree.endpoint_by_id.bind($scope.devicetree);
		$scope.setView("sensors");

		$scope.devicetree.on('update', function(update) {
			Socket.emit('devicetree_update', update);
			console.log(update);
		});

		$scope.devicetree.on('delete', function(deletion) {
			Socket.emit('devicetree_delete', deletion);
		});

		lastTreeUpdateReceivedAt = Date.now();
	});

	Socket.on('devicetree_update', function(update) {
		$scope.devicetree.applyUpdate(update);
		lastTreeUpdateReceivedAt = Date.now();
		updateDisplay();
	});

	Socket.on('devicetree_delete', function(deletion) {
		$scope.devicetree.applyDeletion(deletion);
		lastTreeUpdateReceivedAt = Date.now();
		updateDisplay();
	});

	Socket.emit('devicetree_request_init');
	updateDisplay();
}]);

