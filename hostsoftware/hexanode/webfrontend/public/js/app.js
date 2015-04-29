
angular.module('hexanode', [
	'ng-socket',
	'gauges',
	'i18n',
	'controls'
])
//Create a hexabusclient service  (see https://docs.angularjs.org/guide/providers)
.service('HexabusClient', ['Socket', window.HexabusClient])
.factory('DeviceTree', ['Socket', function DeviceTreeFactory(Socket) {
	var devicetree = new window.DeviceTree();
	Socket.emit('devicetree_request_init');

	Socket.on('devicetree_init', function(json) {
		devicetree.fromJSON(json);
	});

	Socket.on('devicetree_update', function(update) {
		if(update.statemachines !== undefined) {
			console.log(update);
		}
		devicetree.applyUpdate(update);
	});

	Socket.on('devicetree_delete', function(deletion) {
		devicetree.applyDeletion(deletion);
	});

	return devicetree;
}])
.controller('healthController', ['$scope', 'Socket', function($scope, Socket) {
	$scope.errorState = false;

	window.socket = Socket;

	Socket.on('health_update', function(state) {
		$scope.errorState = state;
	});
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
