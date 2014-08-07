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
	$scope.views = all_views;

	$scope.current_view = {};
	$scope.hide_unitless = true;

	$scope.setView = function(view) {
		if (view == "sensors") {
			$("#view-name").text(Lang.pack["dashboard"]["all-sensors"]);
			$scope.current_view = {
				view: "sensors",
				endpoints: $scope.sensorList
			};
			$scope.hide_unitless = true;
		} else if (view == "actors") {
			$("#view-name").text(Lang.pack["dashboard"]["all-actors"]);
			$scope.current_view = {
				view: "actors",
				endpoints: $scope.actorList
			};
			$scope.hide_unitless = false;
		} else {
			$("#view-name").text(view.name);
			$scope.hide_unitless = false;

			$scope.current_view = {
				view: view,
				endpoints: []
			};
			view.devices.forEach(function(id) {
				if ($scope.sensorList[id]) {
					$scope.current_view.endpoints.push($scope.sensorList[id]);
				} else if ($scope.actorList[id]) {
					$scope.current_view.endpoints.push($scope.actorList[id]);
				}
			});
		}
	};

	$scope.setView("sensors");

	var updateDisplayScheduled;
	var updateDisplay = function() {
		if (updateDisplayScheduled) {
			$timeout.cancel(updateDisplayScheduled);
		}

		updateDisplayScheduled = $timeout(function() {
			lastSensorValueReceivedAt = new Date();
			keepLastUpdateCurrent();

			if (pendingUpdateControl) {
				pendingUpdateControl.focus();
			}

			if ($scope.current_view.view == "sensors" || $scope.current_view.view == "actors") {
				$scope.setView($scope.current_view.view);
			}
		}, 100);
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
			id: endpoint.id,
			type: endpoint.type,
			value: data.value
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
			target[ep.id] = {
				ep_desc: {},
				associated: {}
		 	};
			new_ep = true;
		}
		entry = target[ep.id];

		for (var key in ep) {
			entry.ep_desc[key] = ep[key];
		}
		if (new_ep) {
			entry.ep_desc.value = 0;
			entry.ep_desc.has_value = false;

			var associate = function(target) {
				if (target.ep_desc.ip == entry.ep_desc.ip) {
					var master, slave;
					if (target.ep_desc.eid == 2 && entry.ep_desc.eid == 1) {
						master = target;
						slave = entry;
					} else if (target.ep_desc.eid == 1 && entry.ep_desc.eid == 2) {
						master = entry;
						slave = target;
					}
					if (master && slave) {
						master.associated[slave.ep_desc.id] = {
							id: slave.ep_desc.id,

							ep: slave
						};
					}
				}
			};
			for (var key in $scope.sensorList) {
				if (key.substr(0, 1) != "$") {
					associate($scope.sensorList[key]);
				}
			}
			for (var key in $scope.actorList) {
				if (key.substr(0, 1) != "$") {
					associate($scope.actorList[key]);
				}
			}
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

	Socket.on('ep_new', epMetadataHandler, { apply: false });
	Socket.on('ep_metadata', epMetadataHandler, { apply: false });

	Socket.on('device_removed', function(msg) {
		for (var id in $scope.sensorList) {
			if ($scope.sensorList[id].ep_desc.ip == msg.device) {
				delete $scope.sensorList[id];
			}
		}
	});

	Socket.on('ep_update', function(data) {
		var epId = data.ep;
		var ep;
		if (epId in $scope.sensorList) {
			var ep = $scope.sensorList[epId];
		} else if (epId in $scope.actorList) {
			var ep = $scope.actorList[epId];
		} else {
			Socket.emit('ep_request_metadata', epId);
			return;
		}

		ep.ep_desc.value = data.value.value;
		ep.hide = false;
		ep.ep_desc.has_value = true;

		updateDisplay();
	}, { apply: false });

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
.controller('viewConfig', ['$scope', function($scope) {
	$scope.usedEndpoints = [];
	$scope.unusedEndpoints = [];

	var used_hash = {};
	used_hexabus_devices.forEach(function(id) {
		used_hash[id] = true;
	});

	for (var dev in known_hexabus_devices) {
		known_hexabus_devices[dev].eids.forEach(function(ep) {
			if (ep.function != "infrastructure" &&
				(ep.function == "actor" || ep.unit)) {
				if (used_hash[ep.id]) {
					used_hash[ep.id] = ep;
				} else {
					$scope.unusedEndpoints.push(ep);
				}
			}
		});
	}

	used_hexabus_devices.forEach(function(id) {
		$scope.usedEndpoints.push(used_hash[id]);
	});

	$(".device-list > tbody").sortable({
		cursorAt: {
			left: 5,
			top: 5
		},
		connectWith: ".device-list > tbody"
	});

	$scope.doneClick = function() {
		var view_content = [];

		var devices = $(".devices-for-view *[data-endpoint-id]");
		devices.each(function() {
			view_content.push($(this).data("endpoint-id"));
		});
		$("#device-order").attr("value", JSON.stringify(view_content));
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
	}

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
			$scope.devices[data.device].last_update = now();
		}
	});

	var on_ep_metadata = function(ep) {
		if (ep.function == "infrastructure") {
			return;
		}
		var device = ($scope.devices[ep.ip] = $scope.devices[ep.ip] || { ip: ep.ip, eids: [] });

		ep.last_update = Math.round((+new Date(ep.last_update)) / 1000);
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
	}

	$scope.show_rename = function(device) {
		device.rename = true;
	}

	$scope.rename_device = function(device) {
		device.renaming = true;
		var name = device.new_name;
		Socket.emit('device_rename', {device: device.ip, name: name})
	}

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
			var eids = {}
			for(key in msg.device.endpoints) {
				if(msg.device.endpoints[key].function != "infrastructure") {
					eids[key]=msg.device.endpoints[key];
				}
			}
			msg.device.endpoints = eids;
			$scope.device = msg.device;
			$scope.new_name = msg.device.name;
			$scope.found = true;
		}
	}

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
	}

	var on_device_rename_error = function(msg) {
		if($scope.renaming) {
			$scope.renaming = false;
			$scope.rename_error_msg = msg.error.code || msg.error;
			$scope.rename_error_class = "error";
			$scope.rename_error = true;
		}
	}

	$scope.show_rename = function() {
		$scope.rename = true;
	}

	$scope.rename_device = function(ip) {
		$scope.renaming = true;
		var name = $scope.new_name;
		Socket.emit('device_rename', {device: ip, name: name})
	}

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
	}

	$scope.resetForms = function() {
		$scope.masterSlave.resetForm();
		$scope.standbyKiller.resetForm();
		$scope.timetable.resetForm();
		$scope.productionThreshold.resetForm();

		hideAlerts();
	}

	var localizeMessage = function(data,type) {
		localizedMessages = Lang.pack["wizard"]["state-machine"][type] || {};
		console.log(type);
		console.log(data);
		if(data.localization in localizedMessages) {
			message = localizedMessages[data.localization];
			for(name in data.extras) {
				console.log(name + ' replace with ' + data.extras[name]);
				message = message.replace('{' + name + '}', data.extras[name]);
			}
			return message;
		}
		return data.msg;
	}

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
	}

	$scope.masterSlave.resetForm = function() {
		$scope.masterSlave.master = $scope.devices[Object.keys($scope.devices)[0]].ip;
		$scope.masterSlave.threshold = 10;
		$scope.masterSlave.slaves = [];
		$scope.masterSlave.validateForm();
}

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
	}

	$scope.masterSlave.upload = function() {
		hideAlerts();
		$scope.busy = true;
		Socket.emit('master_slave_sm', {master: {ip: $scope.masterSlave.master}, slaves: $scope.masterSlave.slaves, threshold: $scope.masterSlave.threshold});
	};


/*
 * Standbykiller statemachine
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
 * Timetable switch statemachine
 */
	$scope.timetable = {};

	$scope.timetable.addTime = function() {
		var time = moment().local();
		$scope.timetable.times.push({hour:time.hour(),minute:time.minute(),second:time.second()});
		//$scope.timetable.validateForm();
	};

	$scope.timetable.removeTime = function(index) {
		$scope.timetable.times.splice(index,1);
	};

	$scope.timetable.addDatetimePicker = function(element,time) {
            element.datetimepicker({
                pickDate: false,
                format: 'H:mm',
                defaultDate: time.hour+':'+time.minute
            });
	};

	$scope.timetable.upload = function() {;
		hideAlerts();
		$scope.busy = true;
		for (var i in $scope.timetable.times) {
			var time = $('#datetime'+i).data("DateTimePicker").getDate();
			var on = ($('#check'+i).is(":checked")?1:0);
			$scope.timetable.times[i] = {hour:time.hour(),minute:time.minute(),on:on};
		}
		Socket.emit('timetable_sm', {device: {ip: $scope.timetable.device}, times: $scope.timetable.times});
	};

	$scope.timetable.resetForm = function() {
		$scope.timetable.device = $scope.devices[Object.keys($scope.devices)[0]].ip;
		$scope.timetable.times = [];
	};

/*
 * Production threshold statemachine
 */
	$scope.productionThreshold = {};
	$scope.productionThreshold.source_devices = {};
	$scope.productionThreshold.switch_devices = {};

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
			$scope.productionThreshold.source_devices[ip] = $scope.devices[ip];
		}
		if(hasEId($scope.devices[ip],1)) {
			$scope.productionThreshold.switch_devices[ip] = $scope.devices[ip];
		}
	}
	
	$scope.productionThreshold.resetForm = function() {
		$scope.productionThreshold.source = $scope.productionThreshold.source_devices[Object.keys($scope.productionThreshold.source_devices)[0]].ip;
		$scope.productionThreshold.productionThreshold = 10;
		$scope.productionThreshold.offTimeout = 30;
		$scope.productionThreshold.usageThreshold = 10;
		$scope.productionThreshold.onTimeout = 30;
		$scope.productionThreshold.switch = $scope.productionThreshold.switch_devices[Object.keys($scope.productionThreshold.switch_devices)[0]].ip;
	};

	$scope.productionThreshold.upload = function() {
		hideAlerts();
		$scope.busy = true;
		message = {source: $scope.productionThreshold.source,
					productionThreshold: $scope.productionThreshold.productionThreshold,
					offTimeout: $scope.productionThreshold.offTimeout, 
					usageThreshold: $scope.productionThreshold.usageThreshold,
					onTimeout: $scope.productionThreshold.onTimeout,
					switchDevice: $scope.productionThreshold.switch};

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
    }
}])
.directive('ttBoolSwitch', [function() {
	return {
		compile: function(element, attrs, transclude) {
			return {
				post: function(scope, element, attrs, controller) {
					var $elem = $(element);

					$elem.bootstrapSwitch();
				}
			}
		}
	}
}])
.directive('ttDatetimePicker', [function() {
	function link(scope, element, attrs) {
		scope.timetable.addDatetimePicker(element,scope.timetable.times[scope.$index]);
	}

    return {
      link: link
    };
}]);
