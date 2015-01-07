angular.module('hexanode')
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
}]);

