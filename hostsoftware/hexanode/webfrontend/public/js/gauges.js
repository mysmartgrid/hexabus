angular.module('gauges', []).directive('gauge', ["$timeout", function($timeout) {
	return {
		restrict: 'A',
		scope: {
			id: '@',
			min: '@',
			max: '@',
			title: '@titleText',
			value: '@',
			disabled: '=',
			class: '@',
			interpolate_title: '@title',

			titleClick: '&',
			minClick: '&',
			maxClick: '&',

			gauge: '='
		},
		replace: false,
		template: '<div></div>',
		compile: function (element, attrs, transclude) {
			return {
				post: function (scope, element, attrs, controller) {
					var g;

					var watchers = {};

					var makeGauge = function() {
						for (key in watchers) {
							watchers[key]();
						}

						$timeout(function() {
							element.empty();
							watchers = {};
							scope.gauge = g = new JustGage({
								id: scope.id,
								min: +scope.min,
								max: +scope.max,
								title: scope.title,
								value: +scope.value,

								titleClick: function() {
									scope.$apply(function() {
										scope.titleClick();
									});
								},
								minClick: function() {
									scope.$apply(function() {
										scope.minClick();
									});
								},
								maxClick: function() {
									scope.$apply(function() {
										scope.maxClick();
									});
								}
							});

							watchers['value'] = scope.$watch('value', function(val) {
								val = +val;
								if (!isNaN(val)) {
									g.refresh(val);
								}
							});
							watchers['min'] = scope.$watch('min', function(min) {
								min = +min;
								if (!isNaN(min) && min != g.config.min) {
									makeGauge();
								}
							});
							watchers['max'] = scope.$watch('max', function(max) {
								max = +max;
								if (!isNaN(max) && max != g.config.max) {
									makeGauge();
								}
							});
							watchers['title'] = scope.$watch('title', function(title) {
								if (title != g.config.title) {
									makeGauge();
								}
							});
							watchers['disabled'] = scope.$watch('disabled', function(disabled) {
								var elem = $(document.getElementById(scope.id));
								if (disabled && disabled != "false") {
									if (!elem.children(".gauge-disabled").size()) {
										elem.append($('<div class="gauge-disabled"></div>'));
									}
								} else {
									elem.children(".gauge-disabled").remove();
								}
							});
						});
					};

					makeGauge();
				}
			};
		}
	};
}]);
