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
					var elems = attrs.shadow_all
						? $(document.getElementsByClassName("gauge-" + forSensor.ip))
						: $(document.getElementById(forSensor.id));
					var div = $('<div class="spinner-large transient" /><div class="updating-thus-disabled transient" />');
					elems.append(div);

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
				Socket.emit('sensor_change_metadata', { id: sensor.id, data: { minvalue: +value } });
				pendingUpdateControl = null;
			});
	};

	$scope.maxClick = function(sensor) {
		placeUpdateControl(
			sensor,
			sensor.gauge.txtMax,
			null,
			function(value) {
				Socket.emit('sensor_change_metadata', { id: sensor.id, data: { maxvalue: +value } });
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
				text: sensor.name,
				shadow_all: true
			},
			function(value) {
				Socket.emit('device_rename', { device: sensor.ip, name: value });
				pendingUpdateControl = null;
			});
	};

	var sensorMetadataHandler = function(sensor) {
		$scope.sensorList[sensor.id] = sensor;
		$scope.sensorList[sensor.id].value = 0;

		$(document.getElementById(sensor.id)).children(".transient").remove();

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
			var sensorId = data.sensor;
			if (sensorId in $scope.sensorList) {
				$scope.sensorList[sensorId].value = data.value.value;
			} else {
				Socket.emit('sensor_request_metadata', sensorId);
			}

			updateDisplay();
		});
	});

	Socket.on('device_rename_error', function(msg) {
		$(document.getElementsByClassName("gauge-" + msg.device)).children(".transient").remove();
		alert("Could not rename device: communication timed out");
	});

	var waitingLastUpdateRecalc;

	var keepLastUpdateCurrent = function() {
		if (waitingLastUpdateRecalc) {
			$timeout.cancel(waitingLastUpdateRecalc);
		}

		var nextUpdateIn = 5000;
		if (lastSensorValueReceivedAt) {
			var secondsDiff = Math.round((Date.now() - lastSensorValueReceivedAt) / 1000);
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