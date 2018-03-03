var gphoto2 = module.exports = require(__dirname + '/build/Release/gphoto2.node'),
    inherits = require('util').inherits,
    EventEmitter = require('events').EventEmitter;

inherits(gphoto2.GPhoto2, EventEmitter);
inherits(gphoto2.Camera, EventEmitter);
