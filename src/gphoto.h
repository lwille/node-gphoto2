#ifndef __GPHOTO_H
#define __GPHOTO_H
#include "binding.h"
using namespace v8;
#define OLDLIB
class GPCamera;
static Persistent<String> gphoto_test_symbol;
static Persistent<String> gphoto_list_symbol;
static Persistent<String> gphoto_onLog_symbol;
class GPhoto2: public node::ObjectWrap {  
  GPContext * context_;
  
  GPPortInfoList   *portinfolist_;
  CameraAbilitiesList  *abilities_;
  Persistent<Function> logCallBack;
  struct list_request {
      Persistent<Function> cb;
      GPhoto2             *gphoto;
      CameraList	        *list;
      Persistent<Object>	This;
      GPContext           *context;
  };
  static void EIO_List(uv_work_t *req);
  static void EIO_ListCb(uv_work_t *req);  
  public:
    static Persistent<FunctionTemplate> constructor_template;
    Camera * _camera;
    GPhoto2();
    ~GPhoto2();
    static void Initialize(Handle<Object> target);
    static Handle<Value> New(const Arguments &args);
    static Handle<Value> List(const Arguments &args);
    static Handle<Value> Test(const Arguments &args);
    static Handle<Value> SetLogHandler(const Arguments &args);
    GPContext *getContext(){return this->context_;};

    GPPortInfoList *getPortInfoList(){return this->portinfolist_;};
    void setPortInfoList(GPPortInfoList *p){this->portinfolist_=p;};

    CameraAbilitiesList *getAbilitiesList(){return this->abilities_;};
    void setAbilitiesList(CameraAbilitiesList *p){this->abilities_=p;};
    int openCamera(GPCamera *camera);
    int closeCamera(GPCamera *camera);
    static void LogHandler(GPLogLevel level, const char *domain, const char *format, va_list args, void *data);
    
};
#ifdef OLDLIB
extern void onError(GPContext *context, const char *str, va_list args, void *unused);
extern void onStatus(GPContext *context, const char *str,va_list args, void *unused);
#else
static void onError(GPContext *context, const char *str, void *unused);
static void onStatus(GPContext *context, const char *str, void *unused);
#endif
#endif