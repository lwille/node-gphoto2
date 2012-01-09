#ifndef __CAMERA_H
#define __CAMERA_H
  #include <gphoto2/gphoto2-camera.h>
  #include <cstdlib>
  #include <node.h>
  #include <node_buffer.h>
  #include <string>
  #include "gphoto.h"
  using namespace v8;

  static Persistent<String> camera_test_symbol;
  static Persistent<String> camera_getModel_symbol;
  class GPCamera : public node::ObjectWrap {
    std::string model_;
    std::string port_;
    
    GPhoto2    *gphoto_; // maybe we should wrap this to make it persistent
    Camera     *camera_;

    bool isOpen(){return this->camera_ ? true : false;};
    
    struct open_request {
      Persistent<Function> cb;
      char* port;
      char* model;
      Camera  * camera;
      GPhoto2  * gphoto;
    };
    bool open();
    bool close();
    
    public:
      static Persistent<FunctionTemplate> constructor_template;
      GPCamera(GPhoto2* gphoto, std::string  model, std::string  port);
      ~GPCamera();
      static void Initialize(Handle<Object> target);
      static Handle<Value> New(const Arguments& args);
      static Handle<Value> Test(const Arguments &args);
  };
#endif