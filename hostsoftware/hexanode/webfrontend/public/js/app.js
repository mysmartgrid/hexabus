angular.module('dashboard', [
	'ng-socket'
]);

angular.module('dashboard').controller('gaugesDisplayController', ['$scope', 'Socket', '$timeout', function($scope, Socket, $timeout) {
	var lastSensorValueReceivedAt;

	var gauges = {};

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

	var placeUpdateControl = function(element, control, attrs, doneCb) {
		attrs = attrs || {};

		var bound = control.getBBox();
		var box = $('<input type="text" style="position: absolute" />')
			.css("left", bound.x)
			.css("top", bound.y)
			.css("width", attrs.width || (bound.x2 - bound.x))
			.css("height", attrs.height || (bound.y2 - bound.y))
			.attr("value", control.attrs.text);
		var button = $('<input type="button" style="position: absolute" value="OK" />')
			.css("left", bound.x + box.outerWidth())
			.css("top", bound.y);
		for (key in control.attrs) {
			if (typeof key == "string" && key.startsWith("font")) {
				box.css(key, control.attrs[key]);
				button.css(key, control.attrs[key]);
			}
		}

		var cleanup = function() {
			box.remove();
			button.remove();
			pendingUpdateControl = null;
		};

		var accept = function() {
			doneCb(box.prop("value"));
		};

		var waitingCleanup = null;

		box.blur(function() {
			waitingCleanup = $timeout(cleanup, 100);
		});
		box.keydown(function(ev) {
			if (ev.which == 13) {
				accept();
				ev.preventDefault();
				cleanup();
			}
		});
		button.focus(function() {
			if (waitingCleanup) {
				$timeout.cancel(waitingCleanup);
			}
		});
		button.click(function() {
			accept();
			cleanup();
		});

		element.append(box);
		element.append(button);
		box.focus();

		pendingUpdateControl = box;
	};

	var renewGauge = function(sensor) {
		delete gauges[sensor.id];
		$timeout(function() {
			delete $scope.sensorList[sensor.id];
			$timeout(function() {
				$scope.sensorList[sensor.id] = sensor;
			});
		});
	};

	$scope.initGauge = function(sensor) {
		if (!gauges[sensor.id]) {
			gauges[sensor.id] = {
				refresh: function() {}
			};
			$timeout(function() {
				gauges[sensor.id] = new JustGage({
					id: sensor.id,
					value: "" + sensor.value,
					min: sensor.minvalue,
					max: sensor.maxvalue,
					title: sensor.name,
					titleClick: function() {
						placeUpdateControl(
							$(document.getElementById(sensor.id)),
							gauges[sensor.id].txtTitle,
							{
								width: "140px"
							},
							function(value) {
								sensor.name = value;
								renewGauge(sensor);
							});
					},
					minClick: function() {
						placeUpdateControl(
							$(document.getElementById(sensor.id)),
							gauges[sensor.id].txtMin,
							null,
							function(value) {
								sensor.minvalue = +value;
								renewGauge(sensor);
							});
					},
					maxClick: function() {
						placeUpdateControl(
							$(document.getElementById(sensor.id)),
							gauges[sensor.id].txtMax,
							null,
							function(value) {
								sensor.maxvalue = +value;
								renewGauge(sensor);
							});
					}
				});
			}, 0, false);
		}
	}

	var sensorMetadataHandler = function(sensor) {
		$scope.sensorList[sensor.id] = sensor;

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
					if (sensorId in gauges) {
						gauges[sensorId].refresh(data.sensor.value[key]);
					}
				}
			} else {
				Socket.emit('sensor_request_metadata', sensorId);
			}

			updateDisplay();
		});
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
