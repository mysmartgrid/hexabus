angular.module('dashboard', [
]);

angular.module('dashboard').factory('Socket', ['$rootScope', '$location', function($rootScope, $location) {
	var socket = {};

	var connection = io.connect($location.protocol() + "://" + $location.host() + ":" + $location.port());

	var wrap_message = function(callback) {
		return function(data) {
			$rootScope.$apply(function() {
				callback(data);
			});
		};
	};

	socket.on = function(id, callback) {
		connection.on(id, wrap_message(callback));
		return socket;
	}

	socket.emit = connection.emit;

	socket.volatile_emit = function(id, message) {
		return connection.volatile.emit(id, message);
	};

	return socket;
}]);

angular.module('dashboard').controller('gaugesDisplayController', ['$scope', 'Socket', '$timeout', function($scope, Socket, $timeout) {
	var lastSensorValueReceivedAt;

	var gauges = {};

	$scope.lastUpdate = "never";

	$scope.sensorList = {};

	var newGauge = function(sensor) {
		gauges[sensor.id] = new JustGage(sensor);
//  window.onload = function() {
//    for (var g_id in gauge_data) {
//      var current = gauge_data[g_id];
//      console.log("Preparing gauge for " + JSON.stringify(current.title));
//      gauges[g_id] = (new JustGage({
//        id: g_id, 
//        value: "n/a", 
//        min: current.min,
//        max: current.max,
//        title: current.title
//      }));
//    }
//    // get up-to-date values for all sensors
//    socket.emit('sensor_refresh_data');
//    $('a[href=#sensors]').tab('show');
//    $('a[href=#dash]').tab('show');
//  };
	}

	Socket.on('sensor_update', function(data) {
		lastSensorValueReceivedAt = new Date();

		var sensorId = data.sensor.id;
		$scope.sensorList[sensorId] = {
			id: sensorId,
			min: 0,
			max: 100,
			title: sensorId
		};
		for (key in data.sensor.value) {
			$scope.sensorList[sensorId].value = data.sensor.value[key];
		}
		$timeout(function() {
			if (!gauges[sensorId]) {
				gauges[sensorId] = newGauge($scope.sensorList[sensorId]);
			} else {
				gauges[sensorId].refresh();
			}
		});

		keepLastUpdateCurrent();
	});

	var waitingLastUpdateRecalc;

	var keepLastUpdateCurrent = function() {
		if (waitingLastUpdateRecalc) {
			$timeout.cancel(waitingLastUpdateRecalc);
		}

		var nextUpdateIn = 5000;
		if (lastSensorValueReceivedAt) {
			var secondsDiff = (Date.now() - lastSensorValueReceivedAt) / 1000;
			if (secondsDiff == 0) {
				$scope.lastUpdate = "now";
			} else if (secondsDiff < 60) {
				$scope.lastUpdate = Math.round(secondsDiff) + " seconds ago";
			} else {
				$scope.lastUpdate = Math.round(secondsDiff / 60) + " minutes ago";
				nextUpdateIn = 30000;
			}
		}

		waitingLastUpdateRecalc = $timeout(keepLastUpdateCurrent, nextUpdateIn);
	};

//  <% for (var i=0; i < sensorlist.length; i++) { 
//    current=sensorlist[i];
//    %>
//      gauge_data["<%=current.sensor_id%>"]= {
//        title: "<%=current.name%>",
//        value: <%-JSON.stringify(current.values)%>,
//        min: <%-JSON.stringify(current.minvalue)%>,
//        max: <%-JSON.stringify(current.maxvalue)%> 
//      };
//    <% } %>
//
//  $('a[data-toggle="tab"]').on('click', function (e) {
//    socket.emit('sensor_refresh_data');
//  }); 
//
//
//  socket.on('sensor_metadata_refresh', function() {
//    // Trigger a hard reload from the server. This is initiated
//    // by the server e.g. if a new sensor is available.
//    window.location.reload(true);
//  });
}]);
