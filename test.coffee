GPhoto = require "./main"
fs     = require "fs"
gphoto = new GPhoto.GPhoto2()
http   = require 'http'

gphoto.list (cameras)->
  console.log "found #{cameras.length} cameras"
  console.log cameras
  camera = cameras[0] if cameras.length > 0
  if camera
    console.log 'creating preview server'
    server = http.createServer (req, res)->
      camera.getPreview (data)->
        res.writeHead(200, {'Content-Type': 'image/jpg'});
        console.log "sending preview ...."
        res.write data
        res.end()
    server.listen(1337, "127.0.0.1");