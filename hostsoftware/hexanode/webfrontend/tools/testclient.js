var request = require('request');
var Futures = require('futures')

var sequence = Futures.sequence();

sequence
.then(function(next) {
  next();
  request.put({
    url: 'http://10.23.1.253:3000/api/sensor/1',
    headers: {
      'Content-Type': 'application/json'
    },
    body: JSON.stringify({
      name: "sensor_name",
    value: 23
    })
  }, function(error, response, body){
    console.log("Creating a dummy sensor");
    console.log("RESPONSE: "+ error + " " + body);
  })
}).then(function(next) {
  next();
  request.get({
    url: 'http://10.23.1.253:3000/api/sensor' 
  }, function(error, response, body){
    console.log("Listing all sensors");
    console.log("RESPONSE: "+ error + " " + body);
  })
}).then(function(next) {
  next();
  request.get({
    url: 'http://10.23.1.253:3000/api/sensor/1' 
  }, function(error, response, body){
    console.log("Listing sensor 1");
    console.log("RESPONSE: "+ error + " " + body);
  })
}).then(function(next) {
  next();
  request.post({
    url: 'http://10.23.1.253:3000/api/sensor/1',
  headers: {
    'Content-Type': 'application/json'
  },
  body: JSON.stringify({
    'value': 42
  })
  }, function(error, response, body){
    console.log("Updating sensor 1");
    console.log("RESPONSE: "+ error + " " + body);
  })
});
