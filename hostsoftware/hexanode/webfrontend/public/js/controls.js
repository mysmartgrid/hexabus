var UpdateControl = function(element, config) {
	config = config || {};

	var cssApply = function(key, control) {
		if (key in config)
			control.css(key, config[key]);
	};

	var container = $('<div style="position: absolute" />');
	cssApply("left", container);
	cssApply("top", container);

	var box = $('<input type="text" style="position: relative" />');
	cssApply("width", box);
	cssApply("height", box);
	box.attr("value", config.value);
	container.append(box);

	var button = $('<input type="button" value="OK" style="position: absolute" />')
		.css("left", box.css("right"));
	container.append(button);

	if (config.font !== undefined) {
		for(var key in config.font) {
			box.css(key, config.font[key]);
			button.css(key, config.font[key]);
		}
	}



	var finish = function(orderly) {
		container.remove();
		if (!orderly && config.abort) {
			config.abort();
		}
	};

	var accept = function() {
		finish(true);
		if (config.done) {
			config.done(box.prop("value"));
		}
	};

	var focusLostHandler;
	var cancelFocusLost = function() {
		if (focusLostHandler !== undefined) {
			window.clearTimeout(focusLostHandler);
		}
	};
	var focusLost = function() {
		cancelFocusLost();
		focusLostHandler = window.setTimeout(finish, 200);
	};

	var focusedControl = box;
	this.focus = function() {
		focusedControl.focus();
	};

	box.blur(focusLost);
	box.focus(function() {
		cancelFocusLost();
		focusedControl = box;
	});
	box.keydown(function(ev) {
		if (ev.which == 13) {
			accept();
			ev.preventDefault();
		} else if (ev.which == 27) {
			finish(false);
		}
	});

	button.blur(focusLost);
	button.focus(function() {
		cancelFocusLost();
		focusedControl = button;
	});
	button.click(accept);

	element.append(container);
	this.focus();
};

angular.module('controls', [
])
.directive('visAssociatedFunctions', [function() {
	return {
		restrict: 'A',
		scope: {
			endpoint: '=',
			enableEdit: '=',
			editBegin: '=',
			editDone: '='
		},
		replace: false,
		template: 
			'<div>' +
				'<div data-ng-repeat="assoc in endpoint.associated">' +
					'<div data-ng-switch="assoc.eid">' +
						'<div data-ng-switch-when="1">' +
							'<div data-bool-switch="bool-switch" ' +
								'data-endpoint="assoc" ' +
								'data-control="assoc.control" ' +
								'data-enable-edit="enableEdit" ' +
								'data-edit-done="editDone"></div>' +
						'</div>' +
					'</div>' +
				'</div>' +
			'</div>',
		compile: function() {
			return {
				post: function(scope, element, attrs, controller) {
				}
			};
		}
	};
}])
.directive('visGauge', ["$compile", function($compile) {
	return {
		restrict: 'A',
		scope: {
			endpoint: '=',
			control: '=',
			enableEdit: '=',
			hasValue: '=',
			editBegin: '=',
			editDone: '=',
			id: '@',
			cssClass: '@'
		},
		replace: false,
		template:
			'<div class="vis-gauge-tooltip">' +
				'<div data-ng-gauge="gauge" ' +
					'id="{{endpoint.id}}" ' +
					'class="vis-endpoint {{cssClass}}-{{endpoint.ip}}" ' +
					'data-title-text="{{endpoint.name}} [{{endpoint.unit | displayUnit | ' +
						'localize:\'endpoint-registry.units\'}}]" ' +
					'data-min="{{endpoint.minvalue}}" ' +
					'data-max="{{endpoint.maxvalue}}" ' +
					'data-value="{{endpoint.last_value}}" ' +
					'data-gauge="control.gauge" ' +
					'data-disabled="endpoint.timeout" ' +
					'data-min-click="minClick()" ' +
					'data-max-click="maxClick()" ' +
					'data-title-click="titleClick()" ' +
					'data-value-click="valueClick()" ' +
					'title="{{endpoint.description | ' +
						'localize:\'endpoint-registry.descriptions\':endpoint.eid}}"></div>' +
			'</div>',
		compile: function(element, attrs, transclude) {
			return {
				post: function(scope, element, attrs, controller) {
					scope.control = {};
					var pendingUpdateControl = null;

					var tt = element;
					tt.popover({
						html: true,
						title: scope.endpoint.name,
						content: function() {
							var html =
								'<div data-vis-associated-functions="associated-functions" ' +
									'data-endpoint="endpoint" ' +
									'data-enable-edit="enableEdit" ' +
									'data-edit-begin="editBegin" ' +
									'data-edit-done="editDone"></div>';
							return $compile(html)(scope);
						},
						placement: "bottom",
						trigger: "manual"
					});

					var placeUpdateControl = function(control, attrs, done) {
						var element = $(document.getElementById(scope.endpoint.id));
						var bound = control.getBBox();

						attrs = attrs || {};

						var config = {
							left: attrs.left || (bound.x + "px"),
							top: attrs.top || (bound.y + "px"),
							width: attrs.width || ((bound.x2 - bound.x) + "px"),
							height: attrs.height || ((bound.y2 - bound.y) + "px"),

							done: done,
							font: {},

							value: attrs.text || control.attrs.text
						};
						for(var key in control.attrs) {
							if (typeof key == "string" && key.substr(0, 4) == "font") {
								config.font[key] = control.attrs[key];
							}
						}

						pendingUpdateControl = new UpdateControl(element, config);
					};

					scope.control.cover = function() {
						var div = $('<div class="spinner-large transient"></div>' +
							'<div class="updating-thus-disabled transient"></div>');
						$(document.getElementById(scope.endpoint.id))
							.append(div);
					};

					scope.control.coverClass = function() {
						var div = $('<div class="spinner-large transient"></div>' +
							'<div class="updating-thus-disabled transient"></div>');
						$(document.getElementsByClassName(scope.cssClass + "-" + scope.endpoint.ip))
							.append(div);
					};

					scope.control.uncover = function() {
						$(document.getElementById(scope.endpoint.id))
							.children(".transient").remove();
					};

					scope.control.uncoverClass = function() {
						$(document.getElementsByClassName(scope.cssClass + "-" + scope.endpoint.ip))
							.children(".transient").remove();
					};

					scope.minClick = function() {
						if (!scope.enableEdit) {
							return;
						}
						scope.editBegin(scope.endpoint, { target: "minvalue" });
						placeUpdateControl(
								scope.control.gauge.txtMin,
								null,
								function(value) {
									scope.editDone(scope.endpoint, {
										minvalue: +value
									});
									pendingUpdateControl = null;
								});
					};

					scope.maxClick = function() {
						if (!scope.enableEdit) {
							return;
						}
						scope.editBegin(scope.endpoint, { target: "maxvalue" });
						placeUpdateControl(
								scope.control.gauge.txtMax,
								null,
								function(value) {
									scope.editDone(scope.endpoint, {
										maxvalue: +value
									});
									pendingUpdateControl = null;
								});
					};

					scope.titleClick = function() {
						if (!scope.enableEdit) {
							return;
						}
						scope.editBegin(scope.endpoint, { target: "name" });
						placeUpdateControl(
								scope.control.gauge.txtTitle,
								{
									left: "0px",
									width: "140px",
									text: scope.endpoint.name
								},
								function(value) {
									scope.editDone(scope.endpoint, {
										name: value
									});
									pendingUpdateControl = null;
								});
					};

					scope.valueClick = function() {
						element.popover("toggle");
					};

					scope.control.focus = function() {
						if (pendingUpdateControl) {
							pendingUpdateControl.focus();
						}
					};
				}
			};
		}
	};
}])
.directive('boolSwitch', [function() {
	return {
		restrict: 'A',
		scope: {
			endpoint: '=',
			control: '=',
			enableEdit: '=',
			editDone: '=',
			id: '@'
		},
		replace: true,
		template:
			'<div class="make-switch switch-large" data-on="success" data-off="default">' +
				'<input type="checkbox" />' +
			'</div>',
		compile: function(element, attrs, transclude) {
			return {
				post: function(scope, element, attrs, controller) {
					var control = scope.control = {};

					var $elem = $(element);

					$elem.bootstrapSwitch();

					var dropChange = false;

					Object.defineProperties(control, {
						enabled: {
							get: function() { return $elem.bootstrapSwitch('isActive'); },
							set: function(en) { $elem.bootstrapSwitch('setActive', en); }
						},

						value: {
							get: function() { return $elem.bootstrapSwitch('status'); },
							set: function(val) {
								dropChange = true;
								$elem.bootstrapSwitch('setState', val);
								dropChange = false;
							}
						}
					});

					control.cover = function() {
					};
					control.uncover = function() {
					};

					$elem.on('switch-change', function(ev, data) {
						if (!dropChange) {
							scope.editDone(scope.endpoint, {
								value: data.value
							});
						}
					});

					scope.$watch('endpoint.last_value', function(last_value) {
						control.value = last_value;
					});

					control.enabled = scope.enableEdit;
					control.value = scope.endpoint.value;
				}
			};
		}
	};
}])
.directive('visBoolSwitch', [function() {
	return {
		restrict: 'A',
		scope: {
			endpoint: '=',
			control: '=',
			enableEdit: '=',
			editDone: '=',
			id: '@'
		},
		replace: true,
		template:
			'<div class="vis-bool-switch vis-endpoint">' +
				'<div class="title text-center">{{endpoint.name}}</div>' +
				'<div class="title text-center">' +
					'{{endpoint.eid | localize:\'endpoint-registry.descriptions\':endpoint.eid}}' +
				'</div>' +
				'<div class="switch-container">' +
					'<div data-bool-switch="bool-switch" ' +
						'data-endpoint="endpoint" ' +
						'data-control="control" ' +
						'data-enable-edit="enableEdit" ' +
						'data-edit-done="editDone"></div>' +
				'</div>' +
			'</div>',
		compile: function(element, attrs, transclude) {
			return {
				post: function(scope, element, attrs, controller) {
				}
			};
		}
	};
}])
.directive('visEndpoint', [function() {
	return {
		restrict: 'A',
		scope: {
			num: '@',
			endpoint: '=',
			enableEdit: '=',
			editBegin: '=',
			editDone: '=',
		},
		replace: true,
		template:
			'<div data-ng-switch="endpoint.function">' +
				'<div data-ng-switch-when="sensor">' +
					'<div data-vis-gauge="" ' +
						'data-css-class="gauge" ' +
						'data-endpoint="endpoint" ' +
						'data-enable-edit="enableEdit" ' +
						'data-edit-begin="editBegin" ' +
						'data-edit-done="editDone" ' +
						'data-has-value="endpoint.has_value" ' +
						'data-control="endpoint.control">' +
					'</div>' +
				'</div>' +
				'<div data-ng-switch-when="actor">' +
					'<div data-ng-switch="endpoint.type">' +
						'<div data-ng-switch-when="Bool">' +
							'<div data-vis-bool-switch="vis-bool-switch" ' +
								'data-endpoint="endpoint" ' +
								'data-enable-edit="enableEdit" ' +
								'data-edit-done="editDone" ' +
								'data-control="endpoint.control">' +
							'</div>' +
						'</div>' +
					'</div>' +
				'</div>' +
			'</div>',
		compile: function(element, attrs, transclude) {
			return {
				post: function(scope, element, attrs, controller) {
				}
			};
		}
	};
}])
.directive('deviceSelector', ['$timeout', function($timeout) {
	return {
		restrict: 'A',
		scope: {
			deviceList: '=',
			model: '=',
			required: '&',
			change: '&',
			name: '@',
			class: '@',
			id: '@'
		},
		replace: true,
		template:
			'<select data-ng-model="model" data-ng-required="required" data-ng-options="value.ip as value.name for (key,value) in deviceList">' +
			'</select>',

		compile: function(element, attrs, transclude) {
			return {
				post: function(scope, element, attrs, controller) {
					scope.$watch('deviceList', function(deviceList) {
						var inList = deviceList.some(function(dev) {
							return dev.ip === scope.model;
						});
						if(!inList) {
							scope.model = undefined;
						}
					});

					/*
					 * Make sure we change is called AFTER the parent scope was updated.
					 * Simply fowarding change to data-ng-change in the template will NOT work !
					 */
					scope.$watch('model', function() {
						if(scope.change !== undefined) {
							scope.change();
						}
					});
				}
			};
		}
	};
}]);
