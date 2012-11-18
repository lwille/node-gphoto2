process.title = 'node-gphoto2 test program'


GPhoto = require "../build/Release/gphoto2"
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
  #camera.setConfigValue "uilock", 0, (er)->
  #  console.log er if er
  camera.getConfig (er, settings)->
    console.error camera_error:er if er
    console.log settings

app = express()

console.log __dirname
app.use express.static __dirname + '/public'
app.use express.bodyParser()

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
app.put '/settings/:name', (req, res)->
  unless camera
    res.send 404, 'Camera not connected'
  else
    camera.setConfigValue req.params.name, req.body.newValue, (er)->
      if er
        res.send 404, JSON.stringify(er)
      else
        res.send 200

# get configuration
app.get '/settings', (req, res)->
  unless camera
    res.send 404, 'Camera not connected'
  else
    camera.getConfig (er, settings)->
      res.send JSON.stringify(settings)

app.get '/download/*', (req, res)->
  unless camera
    res.send 404, 'Camera not connected'
  else
    if (match = req.url.match /download(.*)$/) and (path = match[1])
      console.log "trying to DL #{path}"
      camera.downloadPicture path, (er, data)->
        if er
          res.send 404, er
        else
          res.header 'Content-Type', 'image/jpeg'
          res.send data

app.get '/takePicture', (req, res)->
  unless camera
    res.send 404, 'Camera not connected'
  else
    camera.takePicture download:(if req.query.download is 'false' then false else true), (er, data)->
      if er
        res.send 404, er
      else
        if req.query.download is 'false'
          console.log data
          res.send "/download"+data
        else
          res.header 'Content-Type', 'image/jpeg'
          res.send data

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