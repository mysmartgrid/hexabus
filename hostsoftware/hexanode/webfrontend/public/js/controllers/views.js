angular.module('hexanode')
.controller('viewConfig', ['$scope', 'Socket', function($scope, Socket) {

	var viewId = window.view_id;

	var devicetree = new window.DeviceTree();

	$scope.usedEndpoints = [];
	$scope.unusedEndpoints = [];

	var updateEndpointLists = function() {
		$scope.usedEndpoints = [];

		var view = devicetree.views[window.view_id];
		for(var epIndex in view.endpoints) {
			var ep = devicetree.endpoint_by_id(view.endpoints[epIndex]);
			if(ep !== undefined) {
				$scope.usedEndpoints.push(ep);
			}
		}

		$scope.unusedEndpoints = [];

		devicetree.forEach(function(device) {
			device.forEachEndpoint(function(ep) {
				if((ep.function == 'sensor' || ep.function == 'actor') && $scope.usedEndpoints.indexOf(ep) == -1) {
					$scope.unusedEndpoints.push(ep);
				}
			});
		});

		console.log($scope.usedEndpoints);
		console.log($scope.unusedEndpoints);
	};

	$(".endpoint-list > tbody").sortable({
		cursorAt: {
			left: 5,
			top: 5
		},
		connectWith: ".endpoint-list > tbody"
	});

	$scope.doneClick = function() {
		var view_content = [];

		var devices = $(".endpoints-for-view *[data-endpoint-id]");
		devices.each(function() {
			view_content.push($(this).data("endpoint-id"));
		});
		$("#endpoint-order").attr("value", JSON.stringify(view_content));
	};


	Socket.on('devicetree_init', function(json) {
		devicetree = new window.DeviceTree(json);

		devicetree.on('update', function(update) {
			Socket.emit('devicetree_update', update);
		});

		devicetree.on('delete', function(deletion) {
			Socket.emit('devicetree_delete', deletion);
		});

		updateEndpointLists();
	});

	Socket.on('devicetree_update', function(update) {
		devicetree.applyUpdate(update);
		updateEndpointLists();
	});

	Socket.on('devicetree_delete', function(deletion) {
		devicetree.applyDeletion(deletion);
		updateEndpointLists();
	});

	Socket.emit('devicetree_request_init');
}]);

