should = require 'should'

GPhoto = require "../main"
{exec} = require 'child_process'
cameras = undefined
path = require 'path'
sinon = require 'sinon'


describe "node-gphoto2", ()->
  before (next)->
    @timeout 5000
    exec "killall PTPCamera", ()=>      
      gphoto = new GPhoto.GPhoto2()
      gphoto.list (list)->
        cameras = list
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
  
  describe 'should be able to take a picture', ()->
    it 'without downloading', (done)->
      cameras[0].takePicture download:false, (er, file)->
        should.not.exist er
        file.should.be.a('string')
        cameras[0].firstPicture = file
        done()
    it 'and download it to a buffer', (done)->
      # @timeout 5000
      cameras[0].takePicture download:true, (er, data)->
        should.not.exist er
        data.should.be.an.instanceOf Buffer
        done()

    it 'and download it to the file system', (done)->
      cameras[0].takePicture download:true, targetPath: '/tmp/foo.XXXXXXX', (er, file)->
        should.not.exist er
        file.should.be.a('string')
        path.exists file, (exists)->
          if exists
            done() 
          else
            done "#{file} does not exist."
    it 'and download it later', (done)->
      cameras[0].downloadPicture cameraPath:cameras[0].firstPicture, targetPath: '/tmp/foo.XXXXXXX', (er, file)->
        should.not.exist er
        file.should.be.a('string')
        path.exists file, (exists)->
          if exists
            done() 
          else
            done "#{file} does not exist."