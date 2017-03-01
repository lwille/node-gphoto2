/* Copyright contributors of the node-gphoto2 project */

#ifndef SRC_BINDING_H_
#define SRC_BINDING_H_

extern "C" {
  #include <gphoto2/gphoto2-camera.h>
  #include <gphoto2/gphoto2-port-log.h>
}

#include <v8.h>
#include <nan.h>

#include <string>
#include <list>
#include <map>
#include <cstdlib>
#include <cctype>
#include <cwctype>
#include <sstream>


#define ASYNC_FN(NAME) static void NAME(uv_work_t* req);
#define ASYNC_CB(NAME) static void NAME(uv_work_t* req, int status);

#define DO_ASYNC(BATON, ASYNC, AFTER)                       \
  uv_work_t* req = new uv_work_t();                         \
  req->data = BATON;                                        \
  uv_queue_work(uv_default_loop(), req, ASYNC, AFTER);

#define REQ_ARGS(N)                                         \
  if (info.Length() < (N)) {                                \
    return Nan::ThrowTypeError("Expected " #N "arguments"); \
  }

#define ADD_PROTOTYPE_METHOD(class, name, method)           \
  class ## _ ## name ## _symbol = NODE_PSYMBOL(#name);      \
  NODE_SET_PROTOTYPE_METHOD(constructor_template, #name, method);

#define REQ_EXT_ARG(I, VAR)                                 \
  if (info.Length() <= (I) || !info[I]->IsExternal()) {     \
    return Nan::ThrowTypeError("Argument " #I " invalid");  \
  }                                                         \
  v8::Local<v8::External> VAR = v8::Local<v8::External>::Cast(info[I]);

#define REQ_FUN_ARG(I, VAR)                                    \
  if (info.Length() <= (I) || !info[I]->IsFunction()) {        \
    return Nan::ThrowTypeError("Argument " #I " must be a function");   \
  }                                                            \
  v8::Local<v8::Function> VAR = v8::Local<v8::Function>::Cast(info[I]);

#define REQ_OBJ_ARG(I, VAR)                                  \
  if (info.Length() <= (I) || !info[I]->IsObject()) {        \
    return Nan::ThrowTypeError("Argument " #I " must be a function"); \
  }                                                          \
  v8::Local<v8::Object> VAR = v8::Local<v8::Array>::Cast(info[I]);

#define REQ_ARR_ARG(I, VAR)                                 \
  if (info.Length() <= (I) || !info[I]->IsArray()) {        \
    return Nan::ThrowTypeError("Argument " #I " must be an Array");  \
  }                                                         \
  v8::Local<v8::Array> VAR = v8::Local<v8::Array>::Cast(info[I]);

#define REQ_STR_ARG(I, VAR)                                 \
  if (info.Length() <= (I) || !info[I]->IsString()) {       \
    return Nan::ThrowTypeError("Argument " #I " must be a string");  \
  }                                                         \
  v8::String::Utf8Value VAR(info[I]->ToString());

#define REQ_INT_ARG(I, VAR)                                 \
  if (info.Length() <= (I) || !info[I]->IsInt32()) {        \
    return Nan::ThrowTypeError("Argument " #I " must be an integer");\
  }                                                         \
  int VAR = info[I]->Int32Value();

// Useful functions taken from library examples, with slight modifications
int open_camera(Camera **camera, std::string model, std::string port,
                GPPortInfoList *portinfolist, CameraAbilitiesList *abilities);
int autodetect(CameraList *list, GPContext *context,
               GPPortInfoList **portinfolist, CameraAbilitiesList  **abilities);
int set_config_value_string(Camera *camera, const char *key, const char *val,
                            GPContext *context);
int get_config_value_string(Camera *camera, const char *key, char **str,
                            GPContext *context);

#endif  // SRC_BINDING_H_
