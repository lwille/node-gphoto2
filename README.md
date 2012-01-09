# node-gphoto2
A Node.js wrapper for libgphoto2
## Prerequisites
* Node.js ~0.6.5
* NPM ~1.1.0
* libgphoto2 ~2.4.11

## Installation
    git clone git@github.com:lwille/node-gphoto2.git
    cd node-gphoto2
    npm i -l
    
## Usage
    gp = require('node-gphoto2');
    GPhoto = new gp.GPhoto2();
    
    // List cameras
    GPhoto.list(function(list){console.log(list[0].model)});
    
    // Take picture with camera object obtained from list()
    emitter = camera.takePicture()
    emitter.on('data', function(data){});
    emitter.on('end', function(){});
    emitter.on('error', function(error){});
