angular.module('schemaForm')
.config(['schemaFormProvider', 'schemaFormDecoratorsProvider', 'sfPathProvider',
	function(schemaFormProvider, schemaFormDecoratorsProvider, sfPathProvider) {
		 schemaFormDecoratorsProvider.addMapping(
  			'bootstrapDecorator',
  			'devicepicker',
  			'/js/devicepicker.html');
}]);
angular.module('hexanode')
.controller('stateMachine', ['$scope', '$http', 'Socket', 'DeviceTree', 'Lang', function($scope, $http, Socket, DeviceTree, Lang) {
	var uuid = window.uuid;

	var translation = Lang.pack['wizard']['state-machine'];

	var addCommonFields = function(json) {
		var start = [
				{
					key: 'name',
					title: translation['name'],
					description: translation['choose-name'],
					feedback: false,
				},
				{
					key : 'comment',
					title: translation['comment'],
					description: translation['choose-comment'],
					type: 'textarea',
					feedback: false,
				},
			];

		var end = [
				{
					type: 'submit',
					title: 'Upload'
				}
		];

		return start.concat(json).concat(end);
	};


	var statemachineClasses = {
		Standbykiller: {
			getUsedDevices : function(machine) {
				return [machine.config.dev].map(getDeviceForIp);
			},
			form: addCommonFields([
				{
					key: 'config.dev',
					title: translation['device'],
					description: translation['choose-device'],
					type: 'devicepicker',
					devices: [],
					updateDevices: function() {
						return DeviceTree.getDevicesWithEndpoint([1,2]);
					},
					feedback: false,
				},
				{
					key : 'config.threshold',
					title: translation['threshold'],
					description: translation['choose-threshold'],
					feedback: false,
				},
				{
					key : 'config.timeout',
					title: translation['timeout'],
					description: translation['choose-timeout'],
					feedback: false,
				}
			]),
		},

		MasterSlave: {
			getUsedDevices: function(machine) {
				return [machine.config.master, machine.config.slave].map(getDeviceForIp);
			},

			form: addCommonFields([
				{
					key: 'config.master',
					title: translation['master-device'],
					description: translation['choose-device'],
					type: 'devicepicker',
					devices: [],
					updateDevices: function() {
						return DeviceTree.getDevicesWithEndpoint([2]);
					},
					feedback: false,
				},
				{
					key : 'config.threshold',
					title: translation['threshold'],
					description: translation['choose-threshold'],
					feedback: false,
				},
				{
					key: 'config.slave',
					title: translation['slave-device'],
					description: translation['choose-device'],
					type: 'devicepicker',
					devices: [],
					updateDevices: function() {
						return DeviceTree.getDevicesWithEndpoint([1]);
					},
					feedback: false,
				},
			])
		},

		Threshold: {
			getUsedDevices: function(machine) {
				return [machine.config.meterDev, machine.config.switchDev].map(getDeviceForIp);
			},
			form: addCommonFields([
				{
					key: 'config.meterDev',
					title: translation['meter-device'],
					description: translation['choose-device'],
					type: 'devicepicker',
					devices: [],
					updateDevices: function() {
						return DeviceTree.getDevicesWithEndpoint([2]);
					},
					feedback: false,
				},
				{
					key: 'config.switchDev',
					title: translation['switch-device'],
					description: translation['choose-device'],
					type: 'devicepicker',
					devices: [],
					updateDevices: function() {
						return DeviceTree.getDevicesWithEndpoint([1,4]);
					},
					feedback: false,
				},

				{
					key : 'config.threshold',
					title: translation['threshold'],
					description: translation['choose-threshold'],
					feedback: false,
				},
				{
					key : 'config.minOnTime',
					title: translation['timeout-min-on'],
					description: translation['choose-timeout-min-on'],
					feedback: false,
				},
				{
					key : 'config.minOffTime',
					title: translation['timeout-min-off'],
					description: translation['choose-timeout-min-off'],
					feedback: false,
				}

			]),
		},

		ProductionThreshold: {
			getUsedDevices: function(machine) {
				return [machine.config.producer, machine.config.consumer].map(getDeviceForIp);
			},

			form: addCommonFields([
				{
					key: 'config.producer',
					title: translation['producer-device'],
					description: translation['choose-device'],
					type: 'devicepicker',
					devices: [],
					updateDevices: function() {
						return DeviceTree.getDevicesWithEndpoint([2]);
					},
					feedback: false,
				},
				{
					key : 'config.prodThreshold',
					title: translation['production-threshold'],
					description: translation['choose-threshold'],
					feedback: false,
				},
				{
					key: 'config.consumer',
					title: translation['consumer-device'],
					description: translation['choose-device'],
					type: 'devicepicker',
					devices: [],
					updateDevices: function() {
						return DeviceTree.getDevicesWithEndpoint([1,2,4]);
					},
					feedback: false,
				},
				{
					key : 'config.conThreshold',
					title: translation['consumption-threshold'],
					description: translation['choose-threshold'],
					feedback: false,
				},
				{
					key : 'config.onTimeout',
					title: translation['timeout-max-on'],
					description: translation['choose-timeout-max-on'],
					feedback: false,
				},
				{
					key : 'config.offTimeout',
					title: translation['timeout-max-off'],
					description: translation['choose-timeout-max-off'],
					feedback: false,
				}
			])
		}
	};


	var localizeSchema = function(schema) {
		var localized;
		if(schema.type === 'object') {
			localized = {
				type: 'object',
				required: schema.required,
				properties: {}
			};

			for(var key in schema.properties) {
				localized.properties[key] = localizeSchema(schema.properties[key]);
			}
		}
		else {
			localized = schema;
			if(schema.localizedValidationMessage !== undefined ||
				translation[schema.localizedValidationMessage] !== undefined) {
					localized.validationMessage = translation[schema.localizedValidationMessage];
					delete localized.localizedValidationMessage;
			}
		}

		return localized;
	};


	var applyToClass = function(machineClass) {
		return function(res){
          	statemachineClasses[machineClass].schema = localizeSchema(res.data);
        };
	};

	for(var machineClass in statemachineClasses) {
		$http.get('schemas/'+machineClass)
       		.then(applyToClass(machineClass));
	}

	var updateDevices = function() {
		for(var index in $scope.form) {
			if($scope.form[index].updateDevices !== undefined) {
				$scope.form[index].devices = $scope.form[index].updateDevices();
			}
		}
	};
	DeviceTree.on('changeApplied', updateDevices);


	var getDeviceForIp = function(ip) {
		if($scope.devicetree.devices[ip] === undefined) {
			throw Error('Unkonwn device ' + ip);
		}
		return $scope.devicetree.devices[ip];
	};

	var getMachineClass = function(machine) {
		if(statemachineClasses[machine.machineClass] === undefined) {
			throw Error('Unkonwn Machine class ' + machine.machineClass);
		}
		return statemachineClasses[machine.machineClass];
	};

	$scope.getUsedDevices = function(machine) {
		return getMachineClass(machine).getUsedDevices(machine);
	};

	$scope.errorClass = function(error) {
		return error ? 'has-error' : 'has-success';
	};

	var hideAlerts = function() {
		$scope.progressAlert.show = false;
		$scope.errorAlert.show = false;
		$scope.successAlert.show = false;
	};

	var localizeMessage = function(data) {
		if(data.localization in translation) {
			var message = translation[data.localization];
			for(var name in data.extras) {
				console.log(name + ' replace with ' + data.extras[name]);
				message = message.replace('{' + name + '}', data.extras[name]);
			}
			if(data.msg !== undefined) {
				console.log('Internal message: ' + data.msg);
			}
			return message;
		}
		console.log('Missing translation: ' + data.localization);
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
			$scope.errorAlert.text = localizeMessage(data.error);
		}
	};

	$scope.form = {};
	$scope.schema = {};
	$scope.model = {};

	$scope.changeClass = function() {
		$scope.schema = statemachineClasses[$scope.machineClass].schema;
		$scope.form = statemachineClasses[$scope.machineClass].form;
		updateDevices();
	};

	$scope.submit = function() {

		$scope.$broadcast('schemaFormValidate');

		if($scope.machineForm.$valid) {
			uploadStatemachines([$scope.model]);
		}
	};

	var statemachineProgress = function(data) {
		hideAlerts();
		$scope.progressAlert.show = true;
		$scope.progressAlert.text = localizeMessage(data);
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
		$scope.errorAlert.text = 'Internal Error: ' + error;
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


	$scope.devicetree = DeviceTree;
	//DeviceTree.on('changeApplied', );
}]);

