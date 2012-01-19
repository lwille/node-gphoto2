# node-gphoto2
A Node.js wrapper for [libgphoto2](http://www.gphoto.org). Useful for remote controlling of DSLRs and other digital cameras supported by [gphoto2](http://www.gphoto.org).

The included test application currently allows you to

* receive a live preview of your camera (if supported). Tested with an EOS 550D on a 2010 iMac at ~17 fps.
* query a list of available configuration options
* query the values of specific configuration options

The test application can be started using ``npm test`` and runs on http://localhost:1337.

## Prerequisites
* Node.js ~0.6.5
* NPM ~1.1.0
* libgphoto2 ~2.4.11 - via ``brew install libgphoto2``, ``apt-get install libgphoto2-2-dev`` or download and build from http://www.gphoto.org/proj/libgphoto2/
* clang compiler

## Installation
    npm install gphoto2

## Usage
```javascript
gp = require('node-gphoto2');
GPhoto = new gp.GPhoto2();

// List cameras
GPhoto.list(function(list){console.log(list[0].model)});

// Take picture with camera object obtained from list()
camera.takePicture(function(data){
  fs.writeFile("picture.jpg", data);
})

// Get preview picture (from AF Sensor, fails silently if unsupported)
camera.getPreview(function(data){
  fs.writeFile("picture.jpg", data);
})
```