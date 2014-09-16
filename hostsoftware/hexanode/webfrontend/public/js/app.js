
angular.module('dashboard', [
	'ng-socket',
	'gauges',
	'i18n',
	'controls'
])
.controller('dashboardController', ['$scope', 'Socket', '$timeout', 'Lang', function($scope, Socket, $timeout, Lang) {
	var hexabusclient = new window.HexabusClient(Socket);

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

		updateDisplayScheduled = $timeout(updateDisplay, 100);
	};

	Socket.on('devicetree_init', function(json) {
		$scope.devicetree = new window.DeviceTree(json);
		$scope.endpointById = $scope.devicetree.endpoint_by_id.bind($scope.devicetree);
		$scope.setView("sensors");

		console.log(json);

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
}])
.controller('viewConfig', ['$scope', function($scope) {
	var known_hexabus_devices = window.known_hexabus_devices;
	var used_hexabus_devices =  window.used_hexabus_devices;
	
	$scope.usedEndpoints = [];
	$scope.unusedEndpoints = [];
	
	for (var dev in known_hexabus_devices) {
		known_hexabus_devices[dev].eids.forEach(function(ep) {
			if (ep.function != "infrastructure" &&
				(ep.function == "actor" || ep.unit)) {
				if ($.inArray(ep.id, used_hexabus_devices) != -1) {
					$scope.usedEndpoints.push(ep);
				} else {
					$scope.unusedEndpoints.push(ep);
				}
			}
		});
	}

	$(".endpoint-list > tbody").sortable({
		cursorAt: {
			left: 5,
			top: 5
		},
		connectWith: ".endpoint-list > tbody"
	});

	$scope.doneClick = function() {
		var view_content = [];

		var devices = $(".endpoints-for-view *[data-endpoint-id]");
		devices.each(function() {
			view_content.push($(this).data("endpoint-id"));
		});
		$("#endpoint-order").attr("value", JSON.stringify(view_content));
	};
}])
.controller('alertController', ['$scope', 'Socket', function($scope, Socket) {
	$scope.error = window.error;
}])
.controller('currentController', ['$scope', 'Socket', function($scope, Socket) {
	$scope.upgrade = function() {
		$scope.upgraded = false;
		$scope.upgrade_error = false;
		$scope.upgrading = true;
		Socket.emit('upgrade');
	};

	Socket.on('upgrade_completed', function(error) {
		$scope.upgrading = false;
		if(error.msg) {
			$scope.upgrade_error = true;
			$scope.upgrade_error_msg = error.msg;	
		} else {
			$scope.upgraded = true;
		}
	});
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
	$scope.error = window.error;
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
					$scope.gotoPage = window.location.origin + '/wizard/success';
					break;
			}
		}
	});
}])
.controller('wizardMSG', ['$scope', 'Socket', function($scope, Socket) {
	$scope.error = window.error;
	Socket.emit('get_activation_code');
	$scope.working = true;

	Socket.on('activation_code', function(data) {
		if (data.error) {
			$scope.failed = true;
			$scope.working = false;
		} else {
			$scope.working = false;
			$scope.registered = true;
			$scope.activationCode = data.code;
			$scope.gotoPage = window.location.origin + '/wizard/success';
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
			if(!$scope.devices[data.device].last_update || data.last_update > $scope.devices[data.device].last_update) {
				$scope.devices[data.device].last_update = data.last_update;
				$scope.devices[data.device].timeout = false;
			}	
		}
	});

	var on_ep_metadata = function(ep) {
		if (ep.function == "infrastructure") {
			return;
		}
		var device = ($scope.devices[ep.ip] = $scope.devices[ep.ip] || { ip: ep.ip, eids: [] });

		console.log(ep.last_update);

		//ep.last_update = Math.round((+new Date(ep.last_update)) / 1000);

		device.name = ep.name;
		if(device.renaming && ep.ip == device.ip) {
			if(device.new_name == ep.name) {
				device.renaming = false;
				device.rename = false;
				device.rename_error_class = "";
				device.rename_error = false;
			}
		} else {
			device.new_name = ep.name;
		}

		if (!device.last_update || device.last_update < ep.last_update) {
			device.last_update = ep.last_update;
		}

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

	var on_device_rename_error = function(msg) {
		var device = $scope.devices[msg.device];
		if(device.renaming) {
			device.renaming = false;
			device.rename_error_msg = msg.error.code || msg.error;
			device.rename_error_class = "error";
			device.rename_error = true;
		}
	};

	$scope.show_rename = function(device) {
		device.rename = true;
	};

	$scope.rename_device = function(device) {
		device.renaming = true;
		var name = device.new_name;
		Socket.emit('device_rename', {device: device.ip, name: name});
	};

	Socket.on('device_rename_error', on_device_rename_error);


	Socket.on('ep_new', on_ep_metadata);
	Socket.on('ep_metadata', on_ep_metadata);

	Socket.on('device_removed', function(msg) {
		for (var id in $scope.devices) {
			if ($scope.devices[id].ip == msg.device) {
				delete $scope.devices[id];
			}
		}
	});

	Socket.emit('devices_enumerate');

	Socket.on('devices_enumerate_done', function() {
		$scope.scanDone = true;
	});

	Socket.on('ep_timeout', function(msg) {
		if($scope.devices[msg.ep.ip]) {
			$scope.devices[msg.ep.ip].timeout = true;
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
}])
.controller('healthController', ['$scope', 'Socket', function($scope, Socket) {
	$scope.errorState = false;

	Socket.on('health_update', function(state) {
		$scope.errorState = state;
	});
}])
.controller('stateMachine', ['$scope', 'Socket', 'Lang', function($scope, Socket, Lang) {
	$scope.devices = window.known_hexabus_devices;

	var hideAlerts = function() {
		$scope.progressAlert.show = false;
		$scope.errorAlert.show = false;
		$scope.successAlert.show = false;
	};

	$scope.resetForms = function() {

		$scope.masterSlave.resetForm();
		$scope.standbyKiller.resetForm();
		$scope.productionThreshold.resetForm();

		hideAlerts();
	};

	var localizeMessage = function(data,type) {
		var localizedMessages = Lang.pack["wizard"]["state-machine"][type] || {};
		console.log(type);
		console.log(data);
		if(data.localization in localizedMessages) {
			var message = localizedMessages[data.localization];
			for(var name in data.extras) {
				console.log(name + ' replace with ' + data.extras[name]);
				message = message.replace('{' + name + '}', data.extras[name]);
			}
			return message;
		}
		return data.msg;
	};

	var on_sm_uploaded = function(data) {
		hideAlerts();
		$scope.busy = false;

		if(data.success) {
			$scope.successAlert.show = true;
		}
		else {
			$scope.errorAlert.show = true;
			$scope.errorAlert.text = localizeMessage(data.error,'errors');
		}
	};
	Socket.on('sm_uploaded', on_sm_uploaded);

	var on_sm_progress = function(data) {
		hideAlerts();
		$scope.progressAlert.show = true;
		$scope.progressAlert.text = localizeMessage(data, 'progress');
		console.log(data.extra);
		if('done' in data.extras && 'count' in data.extras) {
			$scope.progressAlert.percent = data.extras.done / data.extras.count * 100.0;
		}
		else  {
			$scope.progressAlert.percent = 0;
		}
	};
	Socket.on('sm_progress', on_sm_progress);


	$scope.progressAlert = {show : false, text : '', percent : 0};
	$scope.errorAlert = {show: false, text : ''};
	$scope.successAlert = {show: false };

	$scope.buys = false;


/*
 * Master slave statemachine
 */
	$scope.masterSlave = {};
	$scope.masterSlave.slaves = [];
	$scope.masterSlave.maxSlaveCount = Object.keys($scope.devices).length - 1;

	$scope.masterSlave.validateForm = function() {
		var usedIPs = [];
		$scope.masterSlaveForm.$setValidity('disjointDevices',true);
		usedIPs.push($scope.masterSlave.master);
		for(var slave in $scope.masterSlave.slaves) {
			if(usedIPs.indexOf($scope.masterSlave.slaves[slave].ip) > -1) {
				$scope.masterSlaveForm.$setValidity('disjointDevices',false);
			}
			usedIPs.push($scope.masterSlave.slaves[slave].ip);
		}
	};

	$scope.masterSlave.resetForm = function() {
		$scope.masterSlave.master = $scope.devices[Object.keys($scope.devices)[0]].ip;
		$scope.masterSlave.threshold = 10;
		$scope.masterSlave.slaves = [];
		$scope.masterSlave.validateForm();
	};

	$scope.masterSlave.addSlave = function() {
		$scope.masterSlave.slaves.push({ip: $scope.devices[Object.keys($scope.devices)[0]].ip});
		$scope.masterSlave.validateForm();
	};

	$scope.masterSlave.removeSlave = function(slave) {
		var i = $scope.masterSlave.slaves.indexOf(slave);
		if(i > -1) {
			$scope.masterSlave.slaves.splice(i,1);
		}
		$scope.masterSlave.validateForm();
	};

	$scope.masterSlave.upload = function() {
		hideAlerts();
		$scope.busy = true;
		Socket.emit('master_slave_sm', {master: {ip: $scope.masterSlave.master}, slaves: $scope.masterSlave.slaves, threshold: $scope.masterSlave.threshold});
	};


/*
 * Stanbykiller statemachine
 */
	$scope.standbyKiller = {};

	$scope.standbyKiller.upload = function() {
		hideAlerts();
		$scope.busy = true;
		Socket.emit('standbykiller_sm', {device: {ip: $scope.standbyKiller.device}, threshold: $scope.standbyKiller.threshold, timeout: $scope.standbyKiller.timeout});
	};

	$scope.standbyKiller.resetForm = function() {
		$scope.standbyKiller.device = $scope.devices[Object.keys($scope.devices)[0]].ip;
		$scope.standbyKiller.threshold = 10;
		$scope.standbyKiller.timeout = 25;
	};

/*
 * Production threshold statemachine
 */
	$scope.productionThreshold = {};
	$scope.productionThreshold.producer_devices = {};
	$scope.productionThreshold.consumer_devices = {};

	var hasEId = function(device,eid) {
		for(var index in device.eids) {
			if(device.eids[index].eid == eid) {
				return true;
			}
		}
		return false;
	};
	
	for(var ip in $scope.devices) {
		if(hasEId($scope.devices[ip],2)) {
			$scope.productionThreshold.producer_devices[ip] = $scope.devices[ip];
		}
		if(hasEId($scope.devices[ip],1)) {
			$scope.productionThreshold.consumer_devices[ip] = $scope.devices[ip];
		}
	}
	
	$scope.productionThreshold.resetForm = function() {
				
		$scope.productionThreshold.producer = $scope.productionThreshold.producer_devices[Object.keys($scope.productionThreshold.producer_devices)[0]].ip;
		$scope.productionThreshold.productionThreshold = 10;
		$scope.productionThreshold.offTimeout = 30;
		$scope.productionThreshold.usageThreshold = 10;
		$scope.productionThreshold.onTimeout = 30;
		$scope.productionThreshold.consumer = $scope.productionThreshold.consumer_devices[Object.keys($scope.productionThreshold.consumer_devices)[0]].ip;
	};

	$scope.productionThreshold.upload = function() {
		hideAlerts();
		$scope.busy = true;
		var message = {producer: $scope.productionThreshold.producer,
					productionThreshold: $scope.productionThreshold.productionThreshold,
					offTimeout: $scope.productionThreshold.offTimeout, 
					usageThreshold: $scope.productionThreshold.usageThreshold,
					onTimeout: $scope.productionThreshold.onTimeout,
					consumer: $scope.productionThreshold.consumer};

		Socket.emit('productionthreshold_sm', message);
	};

}])
.directive('focusIf', [function () {
    return function focusIf(scope, element, attr) {
        scope.$watch(attr.focusIf, function (newVal) {
            if (newVal) {
                scope.$evalAsync(function() {
                    element.focus();
			element.select();
                });
            }
        });
    };
}]);
