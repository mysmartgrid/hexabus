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
		alert(Lang.pack["rename-error"]["template"].replace("{reason}", Lang.pack["rename-error"]["timeout"]));
	});

	var waitingLastUpdateRecalc;


	var lastUpdateTexts = {
		now: "now",
		seconds: "{ago} seconds ago",
		minutes: "{ago} minutes ago"
	};

	// work around jQuery-localize not being able to cache anything at all
	if (!Lang.pack) {
		Lang.localize($("<span></span>"));
	}
	lastUpdateTexts.now = Lang.pack["dashboard"]["last-update"]["now"];
	lastUpdateTexts.seconds = Lang.pack["dashboard"]["last-update"]["seconds-ago"];
	lastUpdateTexts.minutes = Lang.pack["dashboard"]["last-update"]["minutes-ago"];

	var keepLastUpdateCurrent = function() {
		if (waitingLastUpdateRecalc) {
			$timeout.cancel(waitingLastUpdateRecalc);
		}

		var nextUpdateIn = 5000;
		if (lastSensorValueReceivedAt) {
			var secondsDiff = Math.round((Date.now() - lastSensorValueReceivedAt) / 1000);
			var span = $('#last-update-when');
			var ago, template;
			if (secondsDiff == 0) {
				template = lastUpdateTexts.now;
			} else if (secondsDiff < 60) {
				ago = Math.round(secondsDiff);
				template = lastUpdateTexts.seconds;
			} else {
				ago = Math.round(secondsDiff / 60);
				template = lastUpdateTexts.minutes;
				nextUpdateIn = 30000;
			}
			span.text(template.replace("{ago}", ago));
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

	Socket.on('wizard_configure_step', function(progress) {
		if (progress.error) {
			$scope.failed = true;
		} else {
			switch (progress.step) {
				case 'autoconf':
					$scope.configured = true;
					$scope.stepsDone++;
					break;

				case 'check_msg':
					$scope.connected = true;
					$scope.stepsDone++;
					break;
			}

			if ($scope.stepsDone == 3) {
				$scope.stepsDone = 0;
			}
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
}]);
