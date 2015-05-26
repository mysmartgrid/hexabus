'use strict';

var nconf = require('nconf');

module.exports = function() {
	if(nconf.get('debug')) {
		var stack = new Error().stack;
		var stackframe = stack.split('\n')[2];
		var location = stackframe.replace(/^.*\/(.+:[0-9]+:[0-9]).*$/,'[$1]');
		var args = [location].concat(Array.prototype.slice.call(arguments));
		console.log.apply(console,args);
	}
};
