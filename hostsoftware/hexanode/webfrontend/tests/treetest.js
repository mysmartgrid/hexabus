'use strict';

var assert = require('assert');

var DeviceTree = require('./lib/devicetree');

var tree = new DeviceTree('devicetree.json');
var tree2 = new DeviceTree('devicetree.json');


tree.on('update',function(update) {
	console.log('Update:');
	console.log(JSON.stringify(update,null,'\t'));
	tree2.applyUpdate(update);
});

tree.on('delete',function(deletion) {
	console.log('Delete:');
	console.log(JSON.stringify(deletion,null,'\t'));
	tree2.applyDeletion(deletion);
}); 

var assertEqualTrees = function(msg) {
	assert.equal(JSON.stringify(tree.toJSON(),null,'\t'), JSON.stringify(tree2.toJSON(),null,'\t'), msg);
};

tree.views['9ce90c6a-6ada-4971-b7cd-3268ec2dad4d'].name = 'New Name';
assertEqualTrees('View name change failed');

tree.views['9ce90c6a-6ada-4971-b7cd-3268ec2dad4d'].devices = ['caca::50:c4ff:fe04:8341.2'];
assertEqualTrees('View device list change failed');

var view = tree.addView('FooBarz',['caca::50:c4ff:fe04:8341.42']);
assertEqualTrees('Adding view failed');

tree.removeView(view.id);
assertEqualTrees('Removing view failed');



tree.devices['caca::50:c4ff:fe04:8341'].name = 'A new device Name';
assertEqualTrees('Renaming device failed');

tree.addDevice('caca::42','DoomsdayDevice');
assertEqualTrees('Adding device failed');

tree.removeDevice('caca::42');
assertEqualTrees('Removing device failed');



tree.addDevice('caca::42','DoomsdayDevice2');
tree.devices['caca::42'].addEndpoint(666,{'function' : 'actor', 'description': 'Triggers explosion', 'type' : 1});
assertEqualTrees('Adding actor endpoint failed');

tree.devices['caca::42'].addEndpoint(2,{'function' : 'sensor', 'description': 'Doomsdeay device power consumption', 'type' : 3, 'unit' : 'TerraWatt'});
assertEqualTrees('Adding sensor endpoint failed');

tree.devices['caca::42'].endpoints[2].last_value = 42;
assertEqualTrees('Chaning endpoint value failed');

tree.devices['caca::42'].endpoints[2].maxvalue = 50;
assertEqualTrees('Chaning endpoint max value failed');

tree.devices['caca::42'].endpoints[2].minvalue = 2;
assertEqualTrees('Chaning endpoint min value failed');

tree.devices['caca::42'].removeEndpoint(666);
assertEqualTrees('Removing endpoint failed');


console.log('========================================');
var json1 = JSON.stringify(tree.toJSON(),null,'\t');
console.log(json1);
console.log('========================================');
var json2 = JSON.stringify(tree2.toJSON(),null,'\t');
console.log(json2);
