global[id] ?= require name for id, name of {
  "sinon"
  "fs"
  "GPhoto":"../build/Release/gphoto2"
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
  (buf[0]<<8|buf[1]).toString(16).should.equal 'ffd8' # start of image
  (buf[2]<<8|buf[3]).toString(16).should.equal 'ffe1' # exif header

describe "node-gphoto2", ()->
  before (next)->
    @timeout 5000
    exec "killall PTPCamera", ()=>
      gphoto = new GPhoto.GPhoto2()
      gphoto.onLog 2, (level, domain, message)->
        console.error "%s: %s", domain, message

      gphoto.list (list)->
        cameras = list
        return next "No cameras found" unless cameras[0]
        next()

  it 'should find attached cameras', ()->
    cameras.length.should.be.above 0

  it 'should provide a list of settings', (done)->
    @timeout 4000
    cameras[0].getConfig (er, settings)->
      should.not.exist er
      settings.should.have.property 'main'
      settings.main.should.be.an.instanceOf Object
      settings.main.should.have.property 'children'
      done()

  it 'should allow saving camera settings', (done)->
    async.series [
      (cb)->cameras[0].setConfigValue "capturetarget", 0, cb
      (cb)->cameras[0].setConfigValue "eosviewfinder", 0, cb
      (cb)->cameras[0].setConfigValue "uilock", 1, cb
    ], done

  describe 'should be able to take a picture', ()->
     it 'without downloading', (done)->
       cameras[0].takePicture download:false, (er, file)->
         should.not.exist er
         file.should.be.a 'string'
         cameras[0].firstPicture = file
         done()
     it 'and download it to a buffer', (done)->
       # @timeout 5000
       cameras[0].takePicture download:true, (er, data)->
         should.not.exist er
         data.should.be.an.instanceOf Buffer
         checkJpegHeader data
         done()

     it 'and download it to the file system', (done)->
       cameras[0].takePicture download:true, targetPath: '/tmp/foo.XXXXXXX', (er, file)->
         should.not.exist er
         file.should.be.a 'string'
         fs.exists file, (exists)->
           if exists
             tempfiles.push file
             done()
           else
             done "#{file} does not exist."

     it 'and download it later', (done)->
       cameras[0].downloadPicture cameraPath:cameras[0].firstPicture, targetPath: '/tmp/foo.XXXXXXX', (er, file)->
         should.not.exist er
         file.should.be.a 'string'
         fs.exists file, (exists)->
           if exists
             tempfiles.push file
             done()
           else
             done "#{file} does not exist."

  it "should return a proper error code when something goes wrong", (done)->
    cameras[0].takePicture download:true, targetPath: '/path/does/not/exist/foo.XXXXXXX', (er, file)->
      er.should.be.a 'number'
      done()
  describe 'should be able to take a preview picture', ()->
    before (done)->
      fs.unlink '/tmp/preview.sock', ()->done()
    it 'and send it over a socket', (done)->
      @timeout 25000
      server = net.createServer (c)->
        c.on 'end', ()->
          server.close()
          done()
      server.listen '/tmp/preview.sock'
      server.on 'error', ()->
        log arguments
      server.on 'listening', ()->
        cameras[0].takePicture preview:true, socket:'/tmp/preview.sock', (er)->
          should.not.exist er
          done er

    # clean up our mess :)
    after ()->
      tempfiles.forEach (file)->
        fs.unlinkSync file
