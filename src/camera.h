#ifndef __CAMERA_H
#define __CAMERA_H
  #include <gphoto2/gphoto2-camera.h>
  #include <cstdlib>
  #include <node.h>
  #include <node_buffer.h>
  #include <string>
  #include "gphoto.h"
  using namespace v8;

  static Persistent<String> camera_getConfigValue_symbol;
  static Persistent<String> camera_setConfigValue_symbol;
  static Persistent<String> camera_takePicture_symbol;
  static Persistent<String> camera_getPreview_symbol;
  class GPCamera : public node::ObjectWrap {
    std::string model_;
    std::string port_;
    Persistent<External> gphoto;
    GPhoto2    *gphoto_;
    Camera     *camera_;
  	CameraWidget *config_;
    bool isOpen(){return this->camera_ ? true : false;};
    
    struct take_picture_request {
      Persistent<Function> cb;
      Camera *camera;
      GPCamera *cameraObject;
      GPContext *context;
      const char *data;
      size_t length;
      int ret;  
    };
    struct get_config_request {
      Persistent<Function> cb;
      GPCamera *camera;
      std::string key;
      int ret;
    };
    struct set_config_request {
      Persistent<Function> cb;
      GPCamera *camera;
      std::string key;
      std::string strValue;
      int intValue;
      int ret;
    };

    bool close();
    
    public:
      static Persistent<FunctionTemplate> constructor_template;
      GPCamera(Handle<External> js_gphoto, std::string  model, std::string  port);
      ~GPCamera();
      static void Initialize(Handle<Object> target);
      static Handle<Value> New(const Arguments& args);
      static Handle<Value> GetConfig(const Arguments& args);
      static Handle<Value> GetConfigValue(const Arguments &args);
      static Handle<Value> SetConfigValue(const Arguments &args);
      static Handle<Value> TakePicture(const Arguments &args);
      static Handle<Value> GetPreview(const Arguments& args);
      static void EIO_GetConfig(eio_req *req);
      static int EIO_GetConfigCb(eio_req *req);
      static void EIO_GetConfigValue(eio_req *req);
      static int  EIO_GetConfigValueCb(eio_req *req);
      static void EIO_SetConfigValue(eio_req *req);
      static int  EIO_SetConfigValueCb(eio_req *req);
      static void EIO_TakePicture(eio_req *req);
      static int  EIO_TakePictureCb(eio_req *req);
      static void EIO_CapturePreview(eio_req *req);
      static int EIO_CapturePreviewCb(eio_req *req);
      std::string getPort(){return this->port_;};
      std::string getModel(){return this->model_;};
      void setCamera(Camera *camera){this->camera_=camera;};
      Camera* getCamera();
  };
#endif