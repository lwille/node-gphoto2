GPhoto = require "../main"
fs     = require "fs"
gphoto = new GPhoto.GPhoto2()
http   = require 'http'
process.title = 'node-gphoto2 test program'
preview_listeners=new Array()
express = require 'express'
_gc = ()->gc() if typeof gc is 'function'

requests = {}

camera = undefined

# Fetch a list of cameras and get the first one
gphoto.list (cameras)->
  _gc()
  console.log "found #{cameras.length} cameras"
  console.log cameras
  camera = cameras[0] if cameras.length > 0
  camera.getConfig (er, settings)->
    camera.getConfigValue settings, (er, result)->
      if er
        console.log "getConfigValue error:", er
      else
        console.log result

app = express.createServer()

app.get '/', (req, res)->
  path = './test/test.html'
  fs.stat path, (err, stat)->
    if err
      res.writeHead(404, {'Content-Type': 'text/html'});
      res.end(""+err);
    else
      res.writeHead 200, 'Content-Type':'text/html', 'Content-Length':stat.size
      fs.createReadStream(path).pipe res

logRequests = ()->
  d=Date.parse(new Date())/1000
  if requests[d] > 0
    requests[d]++
  else
    fps = requests[d-1]
    requests = {}
    requests[d]=1
    console.log("#{fps} fps") if fps

app.get '/settings', (req, res)->
  unless camera
    res.send 404, 'Camera not connected'
  else
    camera.getConfig (er, settings)->
      camera.getConfigValue settings, (er, result)->
        res.send JSON.stringify(result)
  
app.get '/preview', (req, res)->
  unless camera
    res.send 404, 'Camera not connected'
  else
    preview_listeners.push res
    if preview_listeners.length is 1
      camera.getPreview (er, data)->
        logRequests()
        tmp = preview_listeners
        preview_listeners = new Array()
        d = Date.parse(new Date())
      
        for listener in tmp
          unless er
            listener.writeHead 200, 'Content-Type': 'image/jpeg', 'Content-Length':data.length                
            listener.write data
          else
            listener.writeHead 500
          listener.end()

app.listen 1337, "0.0.0.0"