angular.module('dashboard', [
	'ng-socket',
	'gauges'
]).controller('gaugesDisplayController', ['$scope', 'Socket', '$timeout', function($scope, Socket, $timeout) {
	var lastSensorValueReceivedAt;

	var pendingUpdateControl = null;

	$scope.lastUpdate = "never";

	$scope.sensorList = {};

	var updateDisplay = function() {
		lastSensorValueReceivedAt = new Date();
		keepLastUpdateCurrent();

		if (pendingUpdateControl) {
			pendingUpdateControl.focus();
		}
	};

	var placeUpdateControl = function(forSensor, control, attrs, done) {
		var element = $(document.getElementById(forSensor.id));
		var bound = control.getBBox();

		attrs = attrs || {};

		var config = {
			left: attrs.left || (bound.x + "px"),
			top: attrs.top || (bound.y + "px"),
			width: attrs.width || ((bound.x2 - bound.x) + "px"),
			height: attrs.height || ((bound.y2 - bound.y) + "px"),

			done: function(value) {
				$scope.$apply(function() {
					done(value);
				});
			},
			font: {},

			value: attrs.text || control.attrs.text
		};
		for (key in control.attrs) {
			if (typeof key == "string" && key.substr(0, 4) == "font") {
				config.font[key] = control.attrs[key];
			}
		}

		pendingUpdateControl = new UpdateControl(element, config);
	};

	$scope.minClick = function(sensor) {
		placeUpdateControl(
			sensor,
			sensor.gauge.txtMin,
			null,
			function(value) {
				sensor.minvalue = +value;
				pendingUpdateControl = null;
			});
	};

	$scope.maxClick = function(sensor) {
		placeUpdateControl(
			sensor,
			sensor.gauge.txtMax,
			null,
			function(value) {
				sensor.maxvalue = +value;
				pendingUpdateControl = null;
			});
	};

	$scope.titleClick = function(sensor) {
		placeUpdateControl(
			sensor,
			sensor.gauge.txtTitle,
			{
				left: "0px",
				width: "140px",
				text: sensor.name
			},
			function(value) {
				Socket.emit('device_rename', { device: sensor.id, newName: value });
				pendingUpdateControl = null;
			});
	};

	var sensorMetadataHandler = function(sensor) {
		$scope.sensorList[sensor.id] = sensor;
		var val = sensor.values[sensor.values.length - 1];
		for (key in val) {
			$scope.sensorList[sensor.id].value = val[key];
		}

		updateDisplay();
	};

	Socket.on('clear_state', function() {
		$scope.sensorList = {};
		gauges = {};
	});

	Socket.on('sensor_new', sensorMetadataHandler);
	Socket.on('sensor_metadata', sensorMetadataHandler);

	Socket.on('sensor_update', function(data) {
		$timeout(function() {
			var sensorId = data.sensor.id;
			if (sensorId in $scope.sensorList) {
				for (key in data.sensor.value) {
					$scope.sensorList[sensorId].value = data.sensor.value[key];
				}
			} else {
				Socket.emit('sensor_request_metadata', sensorId);
			}

			updateDisplay();
		});
	});

	Socket.on('device_renamed', function(data) {
		if (data.device in $scope.sensorList) {
			$scope.sensorList[data.device].name = data.name;
		}
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
