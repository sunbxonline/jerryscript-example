require("./build/bundle.js")
var math = require('./api/math')
console.log(math.add(10,2));

var device = require('./api/device')
console.log("device support camera:" + device.isSupportCamera());
console.log("device support bluetooth:" + device.isSupportBlueTooth());
