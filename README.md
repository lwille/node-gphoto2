# node-gphoto2
A Node.js wrapper for [libgphoto2](http://www.gphoto.org). Useful for remote controlling of DSLRs and other digital cameras supported by [gphoto2](http://www.gphoto.org).


The included test application currently allows you to

* receive a live preview of your camera (if supported). Tested with an EOS 550D on a 2010 iMac at ~17 fps using Chrome (Safari doesn't work and FF is slow as hell).
* query a list of available configuration options
* query the values of specific configuration options

The test suite can be run using ``npm test``. There's also a small test application in test/test-server.coffee which runs on http://localhost:1337 and allows to change camera settings and to
take pictures.

## Prerequisites
* Node.js ~0.8.14
* NPM ~1.1.65
* libgphoto2 ~2.4.14 - via ``brew install libgphoto2``, ``apt-get install libgphoto2-2-dev`` or download and build from http://www.gphoto.org/proj/libgphoto2/
* clang compiler

## Test prerequisites
* async
* coffee-script
* mocha
* should
* sinon

## Test-server prerequisites
* express
* jade
* underscore

## Installation
    npm install gphoto2

## Usage
This example only shows how to achieve certain tasks, it's not meant to be executed without any asynchronous control flow logic.

```javascript
gphoto2 = require('gphoto2');
GPhoto = new gphoto2.GPhoto2();

// List cameras / assign list item to variable to use below options
GPhoto.list(function(list){
  if(list.length === 0) return;
  var camera = list[0];
  console.log("Found", camera.model);

  // get configuration tree
  camera.getConfig(function(er, settings){
    console.log(settings);
  });

  // Set configuration values
  camera.setConfigValue('capturetarget', 1, function(er){
    //...
  })

  // Take picture with camera object obtained from list()
  camera.takePicture({download:true}, function(er, data){
    fs.writeFile("picture.jpg", data);
  });

  // Take picture without downloading immediately
  camera.takePicture({download:false}, function(er, path){
    console.log(path);
  });

  // Take picture and download it to filesystem
  camera.takePicture({
      download:true,
      targetPath:'/tmp/foo.XXXXX'
    }, function(er, tmpname){
      fs.rename(tmpname, './picture.jpg');
  });

  // Download a picture from camera
  camera.downloadPicture({
      cameraPath:'/store_00020001/DCIM/100CANON/IMG_1231.JPG',
      targetPath:'/tmp/foo.XXXXX'
    }, function(er, tmpname){
      fs.rename(tmpname, './picture.jpg');
  });

  // Get preview picture (from AF Sensor, fails silently if unsupported)
  camera.getPreview(function(data){
    fs.writeFile("picture.jpg", data);
  });
});
```
