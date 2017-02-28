# Capture 20 pictures and save them to disk.

GPhoto = require "../"
async  = require "async"
fs     = require "fs"

gphoto = new GPhoto.GPhoto2()

gphoto.list (cameras)->
  if cameras.length and camera = cameras[0]
    async.forEachSeries [0 .. 20], (i, cb)->
      camera.takePicture download:true, (er, data)->
        return cb er if er
        fs.writeFile "series_#{i}.jpg", data, "binary", (er)->
          cb er
    , (er)->
      console.error er if er
      console.log "done."
  else
    console.log "No camera found."
