#ifndef __GPHOTO_H
#define __GPHOTO_H
#include <gphoto2/gphoto2-camera.h>
#include <cstdlib>
#include <node.h>
#include <node_buffer.h>
#include <string.h>

using namespace v8;

static Persistent<String> gphoto_test_symbol;
static Persistent<String> gphoto_list_symbol;
class GPhoto2: public node::ObjectWrap {  
  GPContext * context_;
  
  GPPortInfoList   *portinfolist_;
  CameraAbilitiesList  *abilities_;
  struct list_request {
      Persistent<Function> cb;
      GPhoto2             *gphoto;
      CameraList	        *list;    	
  };
  static void EIO_List(eio_req *req);
  static int EIO_ListAfter(eio_req *req);  
  public:
    static Persistent<FunctionTemplate> constructor_template;
    Camera * _camera;
    GPhoto2();
    static void Initialize(Handle<Object> target);
    static Handle<Value> New(const Arguments &args);
    static Handle<Value> List(const Arguments &args);
    static Handle<Value> Test(const Arguments &args);
    GPContext *getContext(){return this->context_;};

    GPPortInfoList *getPortInfoList(){return this->portinfolist_;};
    void setPortInfoList(GPPortInfoList *p){this->portinfolist_=p;};

    CameraAbilitiesList *getAbilitiesList(){return this->abilities_;};
    void setAbilitiesList(CameraAbilitiesList *p){this->abilities_=p;};

    
};

static void onError (GPContext *context, const char *format, va_list args, void *data);
static void onStatus (GPContext *context, const char *format, va_list args, void *data);

#endif