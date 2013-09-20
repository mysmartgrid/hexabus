angular.module('dashboard', [
]);

angular.module('dashboard').factory('Socket', ['$rootScope', '$location', function($rootScope, $location) {
	var socket = {};

	var connection = io.connect($location.protocol() + "://" + $location.host() + ":" + $location.port());

	var wrap_message = function(callback) {
		return function(data) {
			$rootScope.$apply(function() {
				callback(data);
			});
		};
	};

	socket.on = function(id, callback) {
		connection.on(id, wrap_message(callback));
		return socket;
	}

	socket.emit = function(id, message) {
		connection.emit(id, message);
	};

	socket.volatile_emit = function(id, message) {
		return connection.volatile.emit(id, message);
	};

	return socket;
}]);

angular.module('dashboard').controller('gaugesDisplayController', ['$scope', 'Socket', '$timeout', function($scope, Socket, $timeout) {
	var lastSensorValueReceivedAt;

	var gauges = {};

	$scope.lastUpdate = "never";

	$scope.sensorList = {};

	var updateDisplay = function() {
		lastSensorValueReceivedAt = new Date();
		keepLastUpdateCurrent();
	};

	$scope.initGauge = function(sensor) {
		if (!gauges[sensor.id]) {
			gauges[sensor.id] = {
				refresh: function() {}
			};
			$timeout(function() {
				gauges[sensor.id] = new JustGage({
					id: sensor.id,
					value: "n/a",
					min: sensor.minvalue,
					max: sensor.maxvalue,
					title: sensor.name
				});
			}, 0, false);
		}
	}

	var sensorMetadataHandler = function(sensor) {
		$scope.sensorList[sensor.id] = sensor;

		updateDisplay();
	};

	Socket.on('sensor_new', sensorMetadataHandler);
	Socket.on('sensor_metadata', sensorMetadataHandler);

	Socket.on('sensor_update', function(data) {
		var sensorId = data.sensor.id;
		if (sensorId in $scope.sensorList) {
			for (key in data.sensor.value) {
				$scope.sensorList[sensorId].value = data.sensor.value[key];
				if (sensorId in gauges) {
					gauges[sensorId].refresh(data.sensor.value[key]);
				}
			}
		} else {
			Socket.emit('sensor_request_metadata', sensorId);
		}

		updateDisplay();
	});

	var waitingLastUpdateRecalc;

	var keepLastUpdateCurrent = function() {
		if (waitingLastUpdateRecalc) {
			$timeout.cancel(waitingLastUpdateRecalc);
		}

		var nextUpdateIn = 5000;
		if (lastSensorValueReceivedAt) {
			var secondsDiff = (Date.now() - lastSensorValueReceivedAt) / 1000;
			if (secondsDiff == 0) {
				$scope.lastUpdate = "now";
			} else if (secondsDiff < 60) {
				$scope.lastUpdate = Math.round(secondsDiff) + " seconds ago";
			} else {
				$scope.lastUpdate = Math.round(secondsDiff / 60) + " minutes ago";
				nextUpdateIn = 30000;
			}
		}

		waitingLastUpdateRecalc = $timeout(keepLastUpdateCurrent, nextUpdateIn);
	};
}]);
