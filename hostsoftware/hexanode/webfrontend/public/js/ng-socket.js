angular.module('ng-socket', []).factory('Socket', ['$rootScope', '$location', function($rootScope, $location) {
	var socket = {};

	var connection = io.connect($location.protocol() + "://" + $location.host() + ":" + $location.port());

	var wrap_message = function(callback, options) {
		return function(data) {
			if ($rootScope.$$phase == "$apply" || options.apply != undefined && !options.apply) {
				callback(data);
			} else {
				$rootScope.$apply(function() {
					callback(data);
				});
			}
		};
	};

	socket.on = function(id, callback, options) {
		connection.on(id, wrap_message(callback, options || {}));
		return socket;
	}

	socket.emit = function(id, message) {
		connection.emit(id, message);
	};

	socket.volatile_emit = function(id, message) {
		return connection.volatile.emit(id, message);
	};

	return socket;
}]);
