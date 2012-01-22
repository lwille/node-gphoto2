process.title = 'node-gphoto2 test program'


GPhoto = require "../main"
fs     = require "fs"
gphoto = new GPhoto.GPhoto2()
express = require 'express'
_      = require 'underscore'


_gc = ()->gc() if typeof gc is 'function'

requests = {}
preview_listeners = new Array()

camera = undefined

# Fetch a list of cameras and get the first one
gphoto.list (cameras)->
  _gc()
  console.log "found #{cameras.length} cameras"
  # select first Canon camera
  camera = _(cameras).chain().filter((camera)->camera.model.match /Canon/).first().value()
  # exit if no Canon camera is connected 
  process.exit(-1) unless camera
  # retrieve available configuration settings
  console.log "loading #{camera.model} settings"
  camera.getConfig (er, settings)->
    console.log settings

app = express()

console.log __dirname
app.use express.static __dirname + '/public'

app.engine '.html', require('jade')
app.get '/', (req, res)->
  res.render 'index.html'

logRequests = ()->
  d=Date.parse(new Date())/1000
  if requests[d] > 0
    requests[d]++
  else
    fps = requests[d-1]
    requests = {}
    requests[d]=1
    console.log("#{fps} fps") if fps

# save configuration
app.put '/settings*', (req, res)->

# get configuration
app.get '/settings', (req, res)->
  unless camera
    res.send 404, 'Camera not connected'
  else
    camera.getConfig (er, settings)->
      res.send JSON.stringify(settings)
  
app.get '/preview*', (req, res)->
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