
angular.module('hexanode', [
	'ng-socket',
	'gauges',
	'controls'
])
//Create a hexabusclient service  (see https://docs.angularjs.org/guide/providers)
.service('HexabusClient', ['Socket', window.HexabusClient])
//Create a devicetree service
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
.factory('Lang', function() {
	$.localize("lang", { pathPrefix: "/lang/", language: "de" });

	var localizeBySelector = function(selector) {
		$(selector).localize("lang", { pathPrefix: "/lang/", language: "de" });
	};

	var localizeLastUpdate = function(unix_ts) {
		var lastUpdateTexts = {
			now: "now",
			seconds: "{ago} seconds ago",
			minutes: "{ago} minutes ago"
		};

		lastUpdateTexts.now = $.localize.data.lang["last-update"]["now"];
		lastUpdateTexts.seconds = $.localize.data.lang["last-update"]["seconds-ago"];
		lastUpdateTexts.minutes = $.localize.data.lang["last-update"]["minutes-ago"];

		var secondsDiff = Math.round((Date.now() - 1000 * unix_ts) / 1000);
		var ago, template;
		if (secondsDiff <= 1) {
			template = lastUpdateTexts.now;
		} else if (secondsDiff < 60) {
			ago = Math.round(secondsDiff);
			template = lastUpdateTexts.seconds;
		} else {
			ago = Math.round(secondsDiff / 60);
			template = lastUpdateTexts.minutes;
		}
		return template.replace("{ago}", ago);
	};

	return {
		localize: localizeBySelector,
		pack: $.localize.data.lang,
		localizeLastUpdate: localizeLastUpdate
	};
})
.filter('displayUnit', function() {
	return function(text) {
		if (text === "degC") {
			return "°C";
		} else {
			return text;
		}
	};
})
.filter('localize', function() {
	return function(text, prefix, key) {
		if (key === undefined) {
			key = text;
		}
		var pack = $.localize.data.lang;
		if (prefix !== undefined) {
			var parts = prefix.split('.');
			for (var idx in parts) {
				pack = pack[parts[idx]] || {};
			}
		}
		return pack[key] !== undefined ? pack[key] : text;
	};
})
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
}])
.run(['Lang', function(Lang) {
	Lang.localize('[data-localize]');
}]);
