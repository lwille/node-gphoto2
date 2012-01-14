GPhoto = require "../main"
fs     = require "fs"
gphoto = new GPhoto.GPhoto2()
http   = require 'http'

preview_listeners=new Array()

_gc = ()->gc() if typeof gc is 'function'

requests = {}



deliverIndex = (req, res)->
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
    requests[d]=1
    console.log("#{requests[d-1]} fps")
    delete requests[d-1]

deliverPreview = (camera, req, res)->
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

gphoto.list (cameras)->
  _gc()
  console.log "found #{cameras.length} cameras"
  console.log cameras
  camera = cameras[0] if cameras.length > 0
  if camera
    _gc()    
    console.log 'creating preview server'
    server = http.createServer (req, res)->
      if req.url == '/'
        deliverIndex req, res
      else
        deliverPreview camera, req, res
        _gc()
    server.listen(1337, "0.0.0.0");