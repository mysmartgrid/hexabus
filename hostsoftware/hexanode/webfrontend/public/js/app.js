
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
	$scope.views = window.all_views;

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
		if (data.minvalue !== undefined) {
			entry.control.cover();
			Socket.emit('ep_change_metadata', { id: endpoint.id, data: { minvalue: data.minvalue } });
		} else if (data.maxvalue !== undefined) {
			entry.control.cover();
			Socket.emit('ep_change_metadata', { id: endpoint.id, data: { maxvalue: data.maxvalue } });
		} else if (data.name !== undefined) {
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
		console.log(endpoint);
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
			for (var sensorKey in $scope.sensorList) {
				if (sensorKey.substr(0, 1) != "$") {
					associate($scope.sensorList[sensorKey]);
				}
			}
			for (var actorKey in $scope.actorList) {
				if (actorKey.substr(0, 1) != "$") {
					associate($scope.actorList[actorKey]);
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
			ep = $scope.sensorList[epId];
		} else if (epId in $scope.actorList) {
			ep = $scope.actorList[epId];
		} else {
			Socket.emit('ep_request_metadata', epId);
			return;
		}

		ep.ep_desc.value = data.value;
		ep.disabled = false;
		ep.ep_desc.has_value = true;

		updateDisplay();
	}, { apply: false });

	Socket.on('ep_timeout', function(msg) {
		if (msg.ep.function == "sensor" && $scope.sensorList[msg.ep.id]) {
			$scope.sensorList[msg.ep.id].disabled = true;
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

		$scope.simpleSwitch.resetForm();
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
 * Simple switch statemachine
 */

	$scope.simpleSwitch = {};
	$scope.simpleSwitch.buttons = {};
	$scope.simpleSwitch.actors = [];

	var makeButton = function(ip, eid, name, value) {
			$scope.simpleSwitch.buttons[ip + '.' + eid + '#' + value] = {'ip' : ip, 
																		'eid' : eid, 
																		'name': name, 
																		'value':  value};
	};

	var hexanodeIP = 'fd00:9::50:c4ff:fe04:26a';
	var midiButtonEid = 25;

	makeButton(hexanodeIP,midiButtonEid,'Hexanode - Midipad Button 1',36);
	makeButton(hexanodeIP,midiButtonEid,'Hexanode - Midipad Button 2',37);
	makeButton(hexanodeIP,midiButtonEid,'Hexanode - Midipad Button 3',40);
	makeButton(hexanodeIP,midiButtonEid,'Hexanode - Midipad Button 4',41);

	for(var ip in $scope.devices) {
		var device = $scope.devices[ip];
		for(var eid in device.eids) {
			var endpoint = device.eids[eid];
			if(endpoint.function == "sensor" && endpoint.type == 1) {
				console.log("Button: " + device.name + " - " + endpoint.description);
				makeButton(device.ip,endpoint.eid, device.name + " - " + endpoint.description, 1);
			}
			else if(endpoint.eid == 1) {
				console.log("Switch: " + device.name + " - " + endpoint.description);
				$scope.simpleSwitch.actors.push(device);
			}
		}
	}

	$scope.simpleSwitch.buttonCount = Object.keys($scope.simpleSwitch.buttons).length;

	$scope.simpleSwitch.resetForm = function() {
		$scope.simpleSwitch.buttonsOn = [];
		$scope.simpleSwitch.buttonsOff = [];
		
		$scope.simpleSwitch.switchDevice = $scope.simpleSwitch.actors[Object.keys($scope.simpleSwitch.actors)[0]].ip;

		$scope.simpleSwitch.validateForm();
	};

	var addButton = function(target) {
		target.push({'id': Object.keys($scope.simpleSwitch.buttons)[0]});
		$scope.simpleSwitch.validateForm();
	};

	$scope.simpleSwitch.addButtonOn = function() {
		addButton($scope.simpleSwitch.buttonsOn);
	};
	
	$scope.simpleSwitch.addButtonOff = function() {
		addButton($scope.simpleSwitch.buttonsOff);
	};

	var removeButton  = function(target, button) {
		var i = target.indexOf(button);
		if(i > -1) {
			target.splice(i,1);
		}
		$scope.simpleSwitch.validateForm();
	};
	
	$scope.simpleSwitch.removeButtonOn = function(button) {
		console.log(button);
		removeButton($scope.simpleSwitch.buttonsOn, button);
	};

	$scope.simpleSwitch.removeButtonOff = function(button) {
		removeButton($scope.simpleSwitch.buttonsOff, button);
	};

	var hasDuplicates = function(target) {
		entries = [];

		for(var key in target) {
			var entry = target[key].id;
			if(entries.indexOf(entry) != -1) {
				return true;
			}
			entries.push(entry);
		}

		return false;
	};

	$scope.simpleSwitch.validateForm = function() {
		$scope.simpleSwitchForm.$setValidity('duplicateButtonOn',true);
		$scope.simpleSwitchForm.$setValidity('duplicateButtonOff',true);

		if(hasDuplicates($scope.simpleSwitch.buttonsOn)) {
			$scope.simpleSwitchForm.$setValidity('duplicateButtonOn',false);
		}

		if(hasDuplicates($scope.simpleSwitch.buttonsOff)) {
			$scope.simpleSwitchForm.$setValidity('duplicateButtonOff',false);
		}

	};

	$scope.simpleSwitch.upload = function() {
		hideAlerts();
		$scope.busy = true;

		
		var buttonsOn = [];
		for(var indexOn in $scope.simpleSwitch.buttonsOn) {
			buttonsOn.push($scope.simpleSwitch.buttons[$scope.simpleSwitch.buttonsOn[indexOn].id]);
		}

		var buttonsOff = [];
		for(var indexOff in $scope.simpleSwitch.buttonsOff) {
			buttonsOff.push($scope.simpleSwitch.buttons[$scope.simpleSwitch.buttonsOff[indexOff].id]);
		}

		console.log($scope.simpleSwitch.switchDevice);
		console.log($scope.simpleSwitch.buttons);
		console.log(buttonsOn);
		console.log(buttonsOff);


		Socket.emit('simpleswitch_sm', {'switchIP' : $scope.simpleSwitch.switchDevice, 
										'buttonsOn' : buttonsOn,
										'buttonsOff' : buttonsOff});
	};


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

/*
 * Default demo statemachine
 */
	$scope.demo = {};
 	$scope.demo.upload = function() {
		hideAlerts();
		$scope.busy = true;
		Socket.emit('demo_sm', {});
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
