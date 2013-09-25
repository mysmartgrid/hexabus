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

	var placeUpdateControl = function(element, control, attrs, doneCb) {
		attrs = attrs || {};

		var bound = control.getBBox();

		var pos = {
			left: attrs.left || (bound.x + "px"),
			top: attrs.top || (bound.y + "px"),
			width: attrs.width || ((bound.x2 - bound.x) + "px"),
			height: attrs.height || ((bound.y2 - bound.y) + "px")
		};
		var container = $('<div style="position: absolute" />')
			.css("left", pos.left)
			.css("top", pos.top);
		var box = $('<input type="text" style="position: relative" />')
			.css("width", pos.width)
			.css("height", pos.height)
			.attr("value", control.attrs.text);
		var button = $('<input type="button" value="OK" style="position: absolute" />')
			.css("left", box.css("right"));
		container.append(box, button);
		console.log(control.attrs);
		for (key in control.attrs) {
			if (typeof key == "string" && key.substr(0, 4) == "font") {
				box.css(key, control.attrs[key]);
				button.css(key, control.attrs[key]);
			}
		}

		var cleanup = function() {
			container.remove();
			pendingUpdateControl = null;
		};

		var accept = function() {
			$scope.$apply(function() {
				doneCb(box.prop("value"));
			});
		};

		var waitingCleanup = null;
		var scheduleCleanup = function() {
			waitingCleanup = $timeout(cleanup, 100);
		};
		var abortCleanup = function(ev) {
			if (waitingCleanup) {
				$timeout.cancel(waitingCleanup);
			}
			pendingUpdateControl = $(ev.target);
		};

		box.blur(scheduleCleanup);
		box.focus(abortCleanup);
		box.keydown(function(ev) {
			if (ev.which == 13) {
				accept();
				ev.preventDefault();
				cleanup();
			}
		});

		button.blur(scheduleCleanup);
		button.focus(abortCleanup);
		button.click(function() {
			accept();
			cleanup();
		});

		element.append(container);
		box.focus();

		pendingUpdateControl = box;
	};

	$scope.minClick = function(sensor) {
		placeUpdateControl(
			$(document.getElementById(sensor.id)),
			sensor.gauge.txtMin,
			null,
			function(value) {
				sensor.minvalue = +value;
			});
	};

	$scope.maxClick = function(sensor) {
		placeUpdateControl(
			$(document.getElementById(sensor.id)),
			sensor.gauge.txtMax,
			null,
			function(value) {
				sensor.maxvalue = +value;
			});
	};

	$scope.titleClick = function(sensor) {
		placeUpdateControl(
			$(document.getElementById(sensor.id)),
			sensor.gauge.txtTitle,
			{
				left: "0px",
				width: "140px"
			},
			function(value) {
				sensor.name = value;
			});
	};

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
