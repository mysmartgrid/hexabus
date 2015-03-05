angular.module('hexanode')
.controller('stateMachine', ['$scope', 'Socket', 'Lang', function($scope, Socket, Lang) {
	$scope.devices = window.known_hexabus_devices;

	$scope.errorClass = function(error) {
		return error ? 'has-error' : 'has-success';
	};

	var generateRandomName = function() {
		return 'sm_' + Math.floor(Math.random() * 10000000).toString(36);
	};

	var hideAlerts = function() {
		$scope.progressAlert.show = false;
		$scope.errorAlert.show = false;
		$scope.successAlert.show = false;
	};

	var stateMachineForms = [];

	$scope.resetForms = function() {
		for(var i in stateMachineForms) {
			stateMachineForms[i].resetForm();
		}

		hideAlerts();
	};

	$scope.updateDevices = function() {
		for(var i in stateMachineForms) {
			stateMachineForms[i].updateDevices();
		}
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

	var statemachineUploaded = function(data) {
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

	var statemachineProgress = function(data) {
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
	Socket.on('statemachine_progress', statemachineProgress);

	Socket.on('_statemachine_error_', function(error) {
		hideAlerts();
		$scope.busy = false;

		console.log(error);
		$scope.errorAlert.show = true;
		$scope.errorAlert.text = "Internal Error: " + error;
	});

	var uploadStatemachines = function(statemachines) {
		hideAlerts();
		$scope.busy = true;

		Socket.emit('update_statemachines', statemachines, statemachineUploaded);
	};

	$scope.progressAlert = {show : false, text : '', percent : 0};
	$scope.errorAlert = {show: false, text : ''};
	$scope.successAlert = {show: false };

	$scope.busy = false;


/*
 * Master slave statemachine
 */
	$scope.masterSlave = {};
	$scope.masterSlave.masterDevices = [];
	$scope.masterSlave.slaveDevices = [];
	$scope.masterSlave.slaves = [];

	$scope.masterSlave.validateForm = function() {
		var usedIPs = [];
		$scope.masterSlaveForm.$setValidity('disjointDevices',true);
		$scope.masterSlaveForm.$setValidity('undefinedDevices',true);
		usedIPs.push($scope.masterSlave.master);
		for(var slave in $scope.masterSlave.slaves) {
			if($scope.masterSlave.slaves[slave].ip === undefined) {
				$scope.masterSlaveForm.$setValidity('undefinedDevices',false);
			}
			if(usedIPs.indexOf($scope.masterSlave.slaves[slave].ip) > -1) {
				$scope.masterSlaveForm.$setValidity('disjointDevices',false);
			}
			usedIPs.push($scope.masterSlave.slaves[slave].ip);
		}
	};

	$scope.masterSlave.resetForm = function() {
		$scope.masterSlave.master = $scope.masterSlave.masterDevices[0].ip;
		$scope.masterSlave.threshold = 10;
		$scope.masterSlave.slaves = [];
		$scope.masterSlave.validateForm();
	};

	$scope.masterSlave.updateDevices = function() {
		$scope.masterSlave.masterDevices = $scope.devicetree.getDevicesWithEndpoint([2]);
		$scope.masterSlave.slaveDevices = $scope.devicetree.getDevicesWithEndpoint([1,4]);

		$scope.masterSlave.validateForm();
	};

	$scope.masterSlave.addSlave = function() {
		$scope.masterSlave.slaves.push({ip: $scope.masterSlave.slaveDevices[0].ip});
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
		var statemachines = [];

		for(var slave in $scope.masterSlave.slaves) {
			var statemachine = {
				name: generateRandomName(),
				machineClass: 'MasterSlave',
				config: {
					master: $scope.masterSlave.master,
					meter: 2,
					threshold : $scope.masterSlave.threshold,
					slave: $scope.masterSlave.slaves[slave].ip,
					relais: 1
				}
			};
			statemachines.push(statemachine);
		}
		uploadStatemachines(statemachines);
	};

	stateMachineForms.push($scope.masterSlave);

/*
 * Stanbykiller statemachine
 */
	$scope.standbyKiller = {};
	$scope.standbyKiller.devices = [];

	$scope.standbyKiller.updateDevices = function() {
		$scope.standbyKiller.devices = $scope.devicetree.getDevicesWithEndpoint([2]);
	};

	$scope.standbyKiller.upload = function() {
		var statemachine = [{
			name: generateRandomName(),
			machineClass: 'Standbykiller',
			config: {
				dev: $scope.standbyKiller.device,
				meter: 2,
				relais : 1,
				threshold : $scope.standbyKiller.threshold,
				button : 4,
				timeout : $scope.standbyKiller.timeout
			},
		}];
		uploadStatemachines(statemachine);
	};

	$scope.standbyKiller.resetForm = function() {
		$scope.standbyKiller.device = $scope.standbyKiller.devices[0].ip;
		$scope.standbyKiller.threshold = 10;
		$scope.standbyKiller.timeout = 25;
	};

	stateMachineForms.push($scope.standbyKiller);


/*
 * Production threshold statemachine
 */
	$scope.productionThreshold = {};
	$scope.productionThreshold.producerDevices = [];
	$scope.productionThreshold.consumerDevices = [];


	$scope.productionThreshold.updateDevices = function() {
		$scope.productionThreshold.producerDevices = $scope.devicetree.getDevicesWithEndpoint([2]);
		$scope.productionThreshold.consumerDevices = $scope.devicetree.getDevicesWithEndpoint([1,2,4]);
	};


	$scope.productionThreshold.resetForm = function() {
		$scope.productionThreshold.producer = $scope.productionThreshold.producerDevices[0].ip;
		$scope.productionThreshold.productionThreshold = 10;
		$scope.productionThreshold.offTimeout = 30;
		$scope.productionThreshold.usageThreshold = 10;
		$scope.productionThreshold.onTimeout = 30;
		$scope.productionThreshold.consumer = $scope.productionThreshold.consumerDevices[0].ip;
	};

	$scope.productionThreshold.upload = function() {
			var statemachine = [{
			name: generateRandomName(),
			machineClass: 'ProductionThreshold',
			config: {
				producer: $scope.productionThreshold.producer,
				prodMeter: 2,
				prodThreshold: $scope.productionThreshold.productionThreshold,
				consumer: $scope.productionThreshold.consumer,
				conMeter: 2,
				conThreshold: $scope.productionThreshold.usageThreshold,
				conRelais: 1,
				conButton: 4,
				onTimeout: $scope.productionThreshold.onTimeout,
				offTimeout: $scope.productionThreshold.offTimeout
			},
		}];
		uploadStatemachines(statemachine);
	};

	stateMachineForms.push($scope.productionThreshold);

/*
 * Threshold statemachine
 */
	$scope.threshold = {};
	$scope.threshold.meterDevices = [];
	$scope.threshold.switchDevices = [];

	$scope.threshold.updateDevices = function() {
		$scope.threshold.meterDevices = $scope.devicetree.getDevicesWithEndpoint([2]);
		$scope.threshold.switchDevices = $scope.devicetree.getDevicesWithEndpoint([1]);
	};

	$scope.threshold.resetForm = function() {
		$scope.threshold.meterDevice = $scope.threshold.meterDevices[0].ip;
		$scope.threshold.threshold = 25;
		$scope.threshold.offDelay = 5;
		$scope.threshold.onDelay = 5;
		$scope.threshold.switchDevice = $scope.threshold.switchDevices[0].ip;
	};

	$scope.threshold.upload = function() {
		var statemachine = [{
			name: generateRandomName(),
			machineClass: 'Threshold',
			config: {
				meterDev: $scope.threshold.meterDevice,
				meter: 2,
				threshold : $scope.threshold.threshold,
				switchDev: $scope.threshold.switchDevice,
				relais : 1,
				button : 4,
				minOnTime : $scope.threshold.onDelay,
				minOffTime : $scope.threshold.offDelay
			},
		}];
		uploadStatemachines(statemachine);
	};
	stateMachineForms.push($scope.threshold);


	Socket.on('devicetree_init', function(json) {
		$scope.devicetree = new window.DeviceTree(json);

		$scope.devicetree.on('update', function(update) {
			Socket.emit('devicetree_update', update);
			console.log(update);
		});

		$scope.devicetree.on('delete', function(deletion) {
			Socket.emit('devicetree_delete', deletion);
		});

		$scope.updateDevices();
	});

	Socket.on('devicetree_update', function(update) {
		$scope.devicetree.applyUpdate(update);
		$scope.updateDevices();
	});

	Socket.on('devicetree_delete', function(deletion) {
		$scope.devicetree.applyDeletion(deletion);
		$scope.updateDevices();
	});

	$scope.devicetree = new window.DeviceTree();
	Socket.emit('devicetree_request_init');
}]);

