/* Copyright contributors of the node-gphoto2 project */

#ifndef SRC_GPHOTO_H_
#define SRC_GPHOTO_H_

#include <unistd.h>
#include <string>
#include "binding.h" // NOLINT

using namespace v8;  // NOLINT

class GPCamera;
static Persistent<String> gphoto_list_symbol;
static Persistent<String> gphoto_onLog_symbol;

class GPhoto2 : public node::ObjectWrap {
  GPContext * context_;

  GPPortInfoList   *portinfolist_;
  CameraAbilitiesList  *abilities_;

  struct log_request {
    int level;
    std::string domain;
    std::string message;
    Persistent<Function> cb;
  };

  struct list_request {
    Persistent<Function> cb;
    GPhoto2 *gphoto;
    CameraList *list;
    Persistent<Object> This;
    GPContext *context;
  };

  static void Async_LogCallback(uv_async_t*, int);
  static void Async_List(uv_work_t *req);
  static void Async_ListCb(uv_work_t *req, int status);

 public:
  static Persistent<FunctionTemplate> constructor_template;
  Camera * _camera;
  GPhoto2();
  ~GPhoto2();
  static void Initialize(Handle<Object> target);
  static NAN_METHOD(New);
  static NAN_METHOD(List);
  static NAN_METHOD(SetLogHandler);
  GPContext *getContext() {
    return this->context_;
  }
  GPPortInfoList *getPortInfoList() {
    return this->portinfolist_;
  }
  void setPortInfoList(GPPortInfoList *p) {
    this->portinfolist_ = p;
  }
  CameraAbilitiesList *getAbilitiesList() {
    return this->abilities_;
  }
  void setAbilitiesList(CameraAbilitiesList *p) {
    this->abilities_ = p;
  }
  int openCamera(GPCamera *camera);
  int closeCamera(GPCamera *camera);
  static void LogHandler(GPLogLevel level, const char *domain,
                         const char *str, void *data);
};

#endif  // SRC_GPHOTO_H_
