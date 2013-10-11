angular.module('dashboard', [
	'ng-socket',
	'gauges',
	'i18n'
])
.controller('gaugesDisplayController', ['$scope', 'Socket', '$timeout', 'Lang', function($scope, Socket, $timeout, Lang) {
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
					var div = $('<div class="spinner-large transient"></div><div class="updating-thus-disabled transient"></div>');
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
		$scope.sensorList[sensor.id].has_value = false;

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
				$scope.sensorList[sensorId].hide = false;
				$scope.sensorList[sensorId].has_value = true;
			} else {
				Socket.emit('sensor_request_metadata', sensorId);
			}

			updateDisplay();
		});
	});

	Socket.on('sensor_timeout', function(msg) {
		if ($scope.sensorList[msg.sensor.id]) {
			$scope.sensorList[msg.sensor.id].hide = true;
		}
	});

	Socket.on('device_rename_error', function(msg) {
		$(document.getElementsByClassName("gauge-" + msg.device)).children(".transient").remove();
		alert(Lang.pack["rename-error"]["template"].replace("{reason}", Lang.pack["rename-error"]["timeout"]));
	});

	var waitingLastUpdateRecalc;

	var keepLastUpdateCurrent = function() {
		if (waitingLastUpdateRecalc) {
			$timeout.cancel(waitingLastUpdateRecalc);
		}

		var nextUpdateIn = 5000;
		if (lastSensorValueReceivedAt) {
			var secondsDiff = Math.round((Date.now() - lastSensorValueReceivedAt) / 1000);
			$('#last-update-when').text(Lang.localizeLastUpdate(lastSensorValueReceivedAt / 1000));
		}

		waitingLastUpdateRecalc = $timeout(keepLastUpdateCurrent, nextUpdateIn);
	};
}])
.controller('wizardConnection', ['$scope', 'Socket', function($scope, Socket) {
	$scope.configure = function() {
		Socket.emit('wizard_configure');
		$scope.checking = true;
		$scope.stepsDone = 1;
	};

	$scope.active = function() {
		return $scope.stepsDone > 0;
	};

	$scope.failed = function() {
		return $scope.autoconf_failed || $scope.msg_failed;
	};

	Socket.on('wizard_configure_step', function(progress) {
		switch (progress.step) {
			case 'autoconf':
				if (!progress.error) {
					$scope.configured = true;
					$scope.stepsDone++;
				} else {
					$scope.autoconf_failed = true;
				}
				break;

			case 'check_msg':
				if (!progress.error) {
					$scope.connected = true;
					$scope.stepsDone++;
				} else {
					$scope.msg_failed = true;
				}
				break;
		}

		if ($scope.stepsDone == 3) {
			$scope.stepsDone = 0;
		}
	});
}])
.controller('wizardActivation', ['$scope', 'Socket', function($scope, Socket) {
	$scope.register = function() {
		Socket.emit('wizard_register');
		$scope.registering = true;
	};

	Socket.on('wizard_register_step', function(progress) {
		if (progress.error) {
			$scope.failed = true;
			$scope.registering = false;
		} else {
			switch (progress.step) {
				case 'register_code':
					$scope.registering = false;
					$scope.registered = true;
					$scope.activationCode = progress.code;
					break;
			}
		}
	});
}])
.controller('devicesList', ['$scope', 'Socket', 'Lang', function($scope, Socket, Lang) {
	$scope.devices = window.known_hexabus_devices;

	$scope.Lang = Lang;

	var now = function() {
		return Math.round((+new Date()) / 1000);
	};

	Socket.on('sensor_update', function(data) {
		if ($scope.devices[data.device]) {
			$scope.devices[data.device].last_update = now();
		}
	});

	var on_sensor_metadata = function(sensor) {
		var device = ($scope.devices[sensor.ip] = $scope.devices[sensor.ip] || { ip: sensor.ip, eids: [] });

		device.name = sensor.name;
		device.last_update = now();

		var eid = {
			eid: parseInt(sensor.eid),
			description: sensor.description,
			unit: sensor.unit
		};

		var found = false;
		for (var key in device.eids) {
			found = found || device.eids[key].eid == eid.eid;
			if (found)
				break;
		}

		if (!found) {
			device.eids.push(eid);
			device.eids.sort(function(a, b) {
				return b.eid - a.eid;
			});
		}
	};

	Socket.on('sensor_new', on_sensor_metadata);
	Socket.on('sensor_metadata', on_sensor_metadata);
}]);
