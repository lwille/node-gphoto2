GPhoto = require("./gphoto2")

gphoto = new GPhoto.GPhoto2()

gphoto.list (cameras)->
  console.log "found #{cameras.length} cameras"
  console.log cameras
  for camera in cameras
    console.log "Taking picture with camera:", camera
    camera.takePicture (res)->
      console.log "Yay!"