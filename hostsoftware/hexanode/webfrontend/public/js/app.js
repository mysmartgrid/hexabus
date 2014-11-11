
angular.module('hexanode', [
	'ng-socket',
	'gauges',
	'i18n',
	'controls'
])
//Create a hexabusclient service  (see https://docs.angularjs.org/guide/providers)
.service('HexabusClient', ['Socket', window.HexabusClient])
.controller('healthController', ['$scope', 'Socket', function($scope, Socket) {
	$scope.errorState = false;

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
