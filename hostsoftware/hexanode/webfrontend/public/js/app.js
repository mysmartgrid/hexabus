angular.module('dashboard', [
	'ng-socket',
	'gauges',
	'i18n',
	'controls'
])
.controller('dashboardController', ['$scope', 'Socket', '$timeout', 'Lang', function($scope, Socket, $timeout, Lang) {
	var lastSensorValueReceivedAt;

	var pendingUpdateControl = null;

	$scope.lastUpdate = "never";

	$scope.sensorList = {};
	$scope.actorList = {};

	var updateDisplay = function() {
		lastSensorValueReceivedAt = new Date();
		keepLastUpdateCurrent();

		if (pendingUpdateControl) {
			pendingUpdateControl.focus();
		}
	};

	$scope.editBegin = function(endpoint, data) {
		pendingUpdateControl = $scope.sensorList[endpoint.id].control;
	};

	var sensorEditDone = function(endpoint, data) {
		var entry = $scope.sensorList[endpoint.id];
		if (data.minvalue != undefined) {
			entry.control.cover();
			Socket.emit('ep_change_metadata', { id: endpoint.id, data: { minvalue: data.minvalue } });
		} else if (data.maxvalue != undefined) {
			entry.control.cover();
			Socket.emit('ep_change_metadata', { id: endpoint.id, data: { maxvalue: data.maxvalue } });
		} else if (data.name != undefined) {
			entry.control.coverClass();
			Socket.emit('device_rename', { device: endpoint.ip, name: data.name });
		}
		pendingUpdateControl = null;
	};

	var actorEditDone = function(endpoint, data) {
		Socket.emit('ep_set', {
			ip: endpoint.ip,
			eid: endpoint.eid,
			type: endpoint.type,
			value: data.value ? 1 : 0
		});
	};

	$scope.editDone = function(endpoint, data) {
		if (endpoint.function == "sensor") {
			sensorEditDone(endpoint, data);
		} else if (endpoint.function == "actor") {
			actorEditDone(endpoint, data);
		}
	};

	var epMetadataHandler = function(ep) {
		var target;

		if (ep.function == "actor") {
			target = $scope.actorList;
		} else if (ep.function == "sensor") {
			target = $scope.sensorList;
		} else {
			return;
		}

		var entry;
		var new_ep = false;
		if (!target[ep.id]) {
			target[ep.id] = { ep_desc: {} };
			new_ep = true;
		}
		entry = target[ep.id];

		for (var key in ep) {
			entry.ep_desc[key] = ep[key];
		}
		if (new_ep) {
			entry.ep_desc.value = 0;
			entry.ep_desc.has_value = false;
		}

		if (entry.control) {
			entry.control.uncover();
		}

		updateDisplay();
	};

	Socket.on('clear_state', function() {
		$scope.sensorList = {};
		gauges = {};
	});

	Socket.on('ep_new', epMetadataHandler);
	Socket.on('ep_metadata', epMetadataHandler);

	Socket.on('device_removed', function(msg) {
		for (var id in $scope.sensorList) {
			if ($scope.sensorList[id].ep_desc.ip == msg.device) {
				delete $scope.sensorList[id];
			}
		}
	});

	Socket.on('ep_update', function(data) {
		$timeout(function() {
			var epId = data.ep;
			if (epId in $scope.sensorList && $scope.sensorList[epId].ep_desc.function == "sensor") {
				$scope.sensorList[epId].ep_desc.value = data.value.value;
				$scope.sensorList[epId].hide = false;
				$scope.sensorList[epId].ep_desc.has_value = true;
			} else {
				Socket.emit('ep_request_metadata', epId);
			}

			updateDisplay();
		});
	});

	Socket.on('ep_timeout', function(msg) {
		if (msg.ep.function == "sensor" && $scope.sensorList[msg.ep.id]) {
			$scope.sensorList[msg.ep.id].hide = true;
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

	Socket.on('ep_update', function(data) {
		if ($scope.devices[data.device]) {
			$scope.devices[data.device].last_update = now();
		}
	});

	var on_ep_metadata = function(ep) {
		if (ep.function == "infrastructure") {
			return;
		}
		var device = ($scope.devices[ep.ip] = $scope.devices[ep.ip] || { ip: ep.ip, eids: [] });

		device.name = ep.name;
		device.last_update = now();

		var eid = {
			eid: parseInt(ep.eid),
			description: ep.description,
			unit: ep.unit
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

	Socket.on('ep_new', on_ep_metadata);
	Socket.on('ep_metadata', on_ep_metadata);

	Socket.on('device_removed', function(msg) {
		for (var id in $scope.devices) {
			if ($scope.devices[id].ip == msg.device) {
				delete $scope.devices[id];
			}
		}
	});

	$scope.remove = function(device) {
		var message = "Do you really want to remove {device}?";

		message = ((Lang.pack["wizard"] || {})["device-list"] || {})["forget-dialog"] || message;

		message = message.replace("{device}", device.name);
		if (confirm(message)) {
			Socket.emit('device_remove', { device: device.ip });
			Socket.emit('device_removed', { device: device.ip });
		}
	};
}]);
