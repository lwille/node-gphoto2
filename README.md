# node-gphoto2

[![Build Status](https://travis-ci.org/lwille/node-gphoto2.svg?branch=master)](https://travis-ci.org/lwille/node-gphoto2)
[![NPM version](https://badge.fury.io/js/gphoto2.svg)](http://badge.fury.io/js/gphoto2)

[Become a Patron](https://www.patreon.com/lwille)

A Node.js wrapper for [libgphoto2](http://www.gphoto.org). Useful for remote controlling of DSLRs and other digital cameras supported by [gphoto2](http://www.gphoto.org).

The included test application currently allows you to

* receive a live preview of your camera (if supported). Tested with an EOS 550D on a 2010 iMac at ~17 fps using Chrome (Safari doesn't work and FF is slow as hell).
* query a list of available configuration options
* query the values of specific configuration options

The test suite can be run using `npm test`. There's also a small test application (`npx coffee examples/server.coffee`) which runs on `http://localhost:1337` and allows to change camera settings and to take pictures.

## Prerequisites

* Node.js: versions 10 (LTS), 12 (LTS), 13 (current)
* libgphoto2: ~2.5.x - via `brew install libgphoto2`, `apt-get install libgphoto2-dev` or download and build from http://www.gphoto.org/proj/libgphoto2/
* pkg-config | dpkg (used for dependency checking)
* clang compiler

## Test/dev prerequisites

* async
* coffee-script
* mocha
* pre-commit
* should
* sinon
* superagent

## Test-server prerequisites

* express
* jade
* underscore

## Installation

After installing the dependencies, just install using:

```
npm install gphoto2
```

If it fails, please thoroughly check the output - link errors usually indicate missing dependencies.
Also, the script tries to detect wether libgphoto2 is correctly installed - if this check fails although you properly installed it, please report :)

## Usage

This example only shows how to achieve certain tasks, it's not meant to be executed without any asynchronous control flow logic.

```javascript
var fs = require('fs');
var gphoto2 = require('gphoto2');
var GPhoto = new gphoto2.GPhoto2();

// Negative value or undefined will disable logging, levels 0-4 enable it.
GPhoto.setLogLevel(1);
GPhoto.on('log', function (level, domain, message) {
  console.log(domain, message);
});

// List cameras / assign list item to variable to use below options
GPhoto.list(function (list) {
  if (list.length === 0) return;
  var camera = list[0];
  console.log('Found', camera.model);

  // get configuration tree
  camera.getConfig(function (er, settings) {
    console.log(settings);
  });

  // Set configuration values
  camera.setConfigValue('capturetarget', 1, function (er) {
    //...
  });

  // Take picture with camera object obtained from list()
  camera.takePicture({download: true}, function (er, data) {
    fs.writeFileSync(__dirname + '/picture.jpg', data);
  });

  // Take picture and keep image on camera
  camera.takePicture({
    download: true,
    keep: true
  }, function (er, data) {
    fs.writeFileSync(__dirname + '/picture.jpg', data);
  });

  // Take picture without downloading immediately
  camera.takePicture({download: false}, function (er, path) {
    console.log(path);
  });

  // Take picture and download it to filesystem
  camera.takePicture({
    targetPath: '/tmp/foo.XXXXXX'
  }, function (er, tmpname) {
    fs.renameSync(tmpname, __dirname + '/picture.jpg');
  });

  // Download a picture from camera
  camera.downloadPicture({
    cameraPath: '/store_00020001/DCIM/100CANON/IMG_1231.JPG',
    targetPath: '/tmp/foo.XXXXXX'
  }, function (er, tmpname) {
    fs.renameSync(tmpname, __dirname + '/picture.jpg');
  });

  // Get preview picture (from AF Sensor, fails silently if unsupported)
  camera.takePicture({
    preview: true,
    targetPath: '/tmp/foo.XXXXXX'
  }, function (er, tmpname) {
    fs.renameSync(tmpname, __dirname + '/picture.jpg');
  });
});
```

## Versioning

This project uses [Semantic versioning](https://github.com/mojombo/semver).

## Contributors

* [Brian McBarron](https://github.com/bmcbarron)
* [Brian White](https://github.com/mscdex)
* [David Noelte](https://github.com/marvin)
* [Leonhardt Wille](https://github.com/lwille)
* [Luigi Pinca](https://github.com/lpinca)
* [Michael Kötter](https://github.com/michaelkoetter)
* [Pawel Rupek](https://github.com/pawelrupek)
* [Philipp Trenz](https://github.com/philipptrenz)
* [Tim Hunt](https://github.com/mitnuh)
* [Sijawusz Pur Rahnama](https://github.com/sija)
* [wolfg](https://github.com/wolfg1969)
