/* Copyright contributors of the node-gphoto2 project */

#ifndef SRC_CAMERA_H_
#define SRC_CAMERA_H_

#include <list>
#include <map>
#include <string>

#include "./binding.h"
#include "./gphoto.h"

/**
 * Class for templated typedef's
 * Initialize as A<Typename>::Tree etc.
 */
template <typename T> class A {
 private:
  A(void) {}
 public:
  typedef std::map<std::string, T> Tree;
};

struct TreeNode {
  CameraWidget* value;
  GPContext *context;
  A<TreeNode>::Tree subtree;
  TreeNode(CameraWidget* value, GPContext *context)
      : value(value), context(context) {}
  TreeNode() : value(NULL), context(NULL) {}
};

typedef std::list<std::string> StringList;

static v8::Persistent<v8::String> camera_getConfig_symbol;
static v8::Persistent<v8::String> camera_getConfigValue_symbol;
static v8::Persistent<v8::String> camera_setConfigValue_symbol;
static v8::Persistent<v8::String> camera_takePicture_symbol;
static v8::Persistent<v8::String> camera_downloadPicture_symbol;

class GPCamera : public Nan::ObjectWrap {
  uv_mutex_t cameraMutex;

  void lock() {
    uv_mutex_lock(&this->cameraMutex);
  }
  void unlock() {
    uv_mutex_unlock(&this->cameraMutex);
  }

  std::string model_;
  std::string port_;
  Nan::Persistent<v8::External> gphoto;
  GPhoto2 *gphoto_;
  Camera *camera_;
  CameraWidget *config_;

  bool isOpen() {
    return this->camera_ ? true : false;
  }

  struct take_picture_request {
    Nan::Callback cb;
    Camera *camera;
    GPCamera *cameraObject;
    CameraFile *file;
    GPContext *context;
    char *data;
    unsigned long length; // NOLINT: Use int16/int64/etc, rather than the C type long
    int ret;
    bool download;
    bool preview;
    bool keep;
    std::string path;
    std::string target_path;
    std::string socket_path;
  };

  struct get_config_request {
    Nan::Callback cb;
    GPCamera  *cameraObject;
    Camera    *camera;
    GPContext *context;
    CameraWidget *root;
    bool simpleSettings;
    int ret;
    A<TreeNode>::Tree settings;
  };

  struct set_config_request {
    Nan::Callback cb;
    GPCamera  *cameraObject;
    Camera    *camera;
    GPContext *context;
    std::string key;
    enum {String, Float, Integer} valueType;
    float fltValue;
    int intValue;
    std::string strValue;
    int ret;
  };

  static int enumConfig(get_config_request* req, CameraWidget *root,
                        A<TreeNode>::Tree *tree);
  static int getConfigWidget(GPContext *context, Camera *camera, std::string name,
                             CameraWidget **child, CameraWidget **rootconfig);
  static int setWidgetValue(set_config_request *req);
  static void takePicture(take_picture_request *req);
  static void capturePreview(take_picture_request *req);
  static void downloadPicture(take_picture_request *req);
  static int getCameraFile(take_picture_request *req, CameraFile **file);
  static v8::Local<v8::Object> convertSettingsToObject(bool minify, GPContext *context, const A<TreeNode>::Tree &node);
  static int convertValueForWidgetType(CameraWidget *child, set_config_request *req, void **value);
  static int setWidgetValue(CameraWidget *child, CameraWidgetType* type, float val);
  static int setWidgetValue(CameraWidget *child, CameraWidgetType* type, int val);
  static int setWidgetValue(CameraWidget *child, CameraWidgetType* type, std::string val);

  bool close();

 public:
  GPCamera(v8::Local<v8::External> js_gphoto, std::string  model, std::string  port);
  ~GPCamera();

  static v8::Local<v8::Value> getWidgetValue(GPContext *context, CameraWidget *widget);

  static Nan::Persistent<v8::Function> constructor;
  static NAN_MODULE_INIT(Initialize);
  static NAN_METHOD(New);
  static NAN_METHOD(GetConfig);
  static NAN_METHOD(GetConfigValue);
  static NAN_METHOD(SetConfigValue);
  static NAN_METHOD(TakePicture);
  static NAN_METHOD(DownloadPicture);

  ASYNC_FN(Async_GetConfig);         // TODO(lwille): Rewrite using NanAsyncWorker
  ASYNC_CB(Async_GetConfigCb);       // TODO(lwille): Rewrite using NanAsyncWorker
  ASYNC_FN(Async_SetConfigValue);    // TODO(lwille): Rewrite using NanAsyncWorker
  ASYNC_CB(Async_SetConfigValueCb);  // TODO(lwille): Rewrite using NanAsyncWorker
  ASYNC_FN(Async_DownloadPicture);   // TODO(lwille): Rewrite using NanAsyncWorker
  ASYNC_FN(Async_Capture);           // TODO(lwille): Rewrite using NanAsyncWorker
  ASYNC_CB(Async_CaptureCb);         // TODO(lwille): Rewrite using NanAsyncWorker

  std::string getPort() {
    return this->port_;
  }

  std::string getModel() {
    return this->model_;
  }

  void setCamera(Camera *camera) {
    this->camera_ = camera;
  }

  Camera* getCamera();
};

#endif  // SRC_CAMERA_H_
