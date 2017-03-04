global[id] ?= require name for id, name of {
  "sinon"
  "fs"
  "GPhoto":"../"
  "child_process"
  "net"
  "async"
}
log = console.log
should = require "should"
{exec} = child_process

cameras = undefined
tempfiles = []

checkJpegHeader = (buf)->
  buf.readUInt16BE(0).toString(16).should.equal 'ffd8' # start of image
  buf.readUInt16BE(2).toString(16).should.equal 'ffe1' # exif header

describe "node-gphoto2", ()->
  before (next)->
    @timeout 10000
    exec "killall PTPCamera", ()=>
      gphoto = new GPhoto.GPhoto2()
      gphoto.setLogLevel 1
      gphoto.on "log", (level, domain, message)->
        console.error "%s: %s", domain, message

      gphoto.list (list)->
        cameras = list
        return next "No cameras found" unless cameras[0]
        next()

  it 'should find attached cameras', ()->
    cameras.length.should.be.above 0

  it 'should provide a list of settings', (done)->
    @timeout 10000
    cameras[0].getConfig (er, settings)->
      try
        should.not.exist er
        settings.should.have.property 'main'
        settings.main.should.be.an.instanceOf Object
        settings.main.should.have.property 'children'
        done()
      catch error
        done error

  it 'should allow saving camera settings', (done)->
    @timeout 10000
    cameras[0].setConfigValue "capturetarget", 1, (er) ->
      should.not.exist er
      done()
#    async.series [
      # (cb)->cameras[0].setConfigValue "eosviewfinder", 0, cb
      # (cb)->cameras[0].setConfigValue "uilock", 1, cb
#    ], done

  describe 'should be able to take a picture', ()->
     it 'without downloading', (done)->
       @timeout 10000
       cameras[0].takePicture download:false, (er, file)->
         try
           should.not.exist er
           file.should.be.a 'string'
           cameras[0].firstPicture = file
           done()
         catch error
           done error

     it 'and download it to a buffer', (done)->
       @timeout 10000
       cameras[0].takePicture download:true, (er, data)->
         try
           should.not.exist er
           data.should.be.an.instanceOf Buffer
           checkJpegHeader data
           done()
         catch error
           done error

     it 'and download it to the file system', (done)->
       @timeout 10000
       cameras[0].takePicture download:true, targetPath: '/tmp/foo.XXXXXXX', (er, file)->
         try
           should.not.exist er
           file.should.be.a 'string'
           fs.exists file, (exists)->
             if exists
               tempfiles.push file
               done()
             else
               done "#{file} does not exist."
         catch error
           done error

     it 'and download it later', (done)->
       @timeout 10000
       cameras[0].downloadPicture cameraPath:cameras[0].firstPicture, targetPath: '/tmp/foo.XXXXXXX', (er, file)->
         try
           should.not.exist er
           file.should.be.a 'string'
           fs.exists file, (exists)->
             if exists
               tempfiles.push file
               done()
             else
               done "#{file} does not exist."
         catch error
           done error

  it "should return a proper error code when something goes wrong", (done)->
    @timeout 10000
    cameras[0].takePicture download:true, targetPath: '/path/does/not/exist/foo.XXXXXXX', (er, file)->
      try
        er.should.be.a 'number'
        done()
      catch error
        done error

  describe 'should be able to take a preview picture', ()->
    before (done)->
      fs.unlink '/tmp/preview.sock', ()->done()
    it 'and send it over a socket', (done)->
      @timeout 10000
      server = net.createServer (c)->
        c.on 'end', ()->
          server.close()
#          done()
      server.listen '/tmp/preview.sock'
      server.on 'error', ()->
        log arguments
      server.on 'listening', ()->
        cameras[0].takePicture preview:true, socket:'/tmp/preview.sock', (er)->
          try
            should.not.exist er
            done()
          catch error
            done error

    # clean up our mess :)
    after ()->
      tempfiles.forEach (file)->
        fs.unlinkSync file
