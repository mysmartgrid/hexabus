angular.module('hexanode')
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
}]);

