/* Copyright contributors of the node-gphoto2 project */

#ifndef SRC_GPHOTO_H_
#define SRC_GPHOTO_H_

#include <unistd.h>
#include <string>
#include <vector>

#include "./binding.h"

class GPCamera;

static v8::Persistent<v8::String> gphoto_list_symbol;
static v8::Persistent<v8::String> gphoto_onLog_symbol;

class GPhoto2 : public Nan::ObjectWrap {
  struct log_request;

  GPContext *context_;
  GPPortInfoList *portinfolist_ = NULL;
  CameraAbilitiesList *abilities_ = NULL;

  uv_async_t asyncLog;
  uv_mutex_t logMutex;
  uv_mutex_t lstMutex;

  int logFuncId = 0;
  int logLevel = 0;
  std::vector<log_request*> logMessages;
  Nan::Callback *emitFuncCb = NULL;

  struct log_request {
    int level;
    std::string domain;
    std::string message;
  };

  struct list_request {
    Nan::Callback cb;
    GPhoto2 *gphoto;
    CameraList *list;
    Nan::Persistent<v8::Object> This;
    GPContext *context;
  };

  static NAUV_WORK_CB(Async_LogCallback);
  static void Async_List(uv_work_t *req);
  static void Async_ListCb(uv_work_t *req, int status);

 public:
  static Nan::Persistent<v8::Function> constructor;
  Camera * _camera;

  GPhoto2();
  ~GPhoto2();

  static NAN_MODULE_INIT(Initialize);
  static NAN_METHOD(New);
  static NAN_METHOD(List);
  static NAN_METHOD(SetLogLevel);

  static void LogHandler(GPLogLevel level, const char *domain, const char *str, void *data);
  static void Async_LogClose(uv_handle_t *handle);

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
};

#endif  // SRC_GPHOTO_H_
