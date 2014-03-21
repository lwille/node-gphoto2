/* Copyright contributors of the node-gphoto2 project */

#ifndef SRC_BINDING_H_
#define SRC_BINDING_H_

extern "C" {
  #include <gphoto2/gphoto2-camera.h>
  #include <gphoto2/gphoto2-port-log.h>
}
#include <node.h>
#include <node_buffer.h>
#include <cstdlib>
#include <string>
#include <list>
#include <map>

#include "cvv8/v8-convert.hpp"

#define ASYNC_FN(NAME) static void NAME(uv_work_t* req);
#define ASYNC_CB(NAME) static void NAME(uv_work_t* req, int status);

#define DO_ASYNC(BATON, ASYNC, AFTER)                       \
  uv_work_t* req = new uv_work_t();                         \
  req->data = BATON;                                        \
  uv_queue_work(uv_default_loop(), req, ASYNC, AFTER);

#define REQ_ARGS(N)                                         \
  if (args.Length() < (N))                                  \
    return ThrowException(Exception::TypeError(             \
      String::New("Expected " #N "arguments")));

#define ADD_PROTOTYPE_METHOD(class, name, method)                 \
  class ## _ ## name ## _symbol = NODE_PSYMBOL(#name);            \
  NODE_SET_PROTOTYPE_METHOD(constructor_template, #name, method);

#define REQ_EXT_ARG(I, VAR)                                 \
  if (args.Length() <= (I) || !args[I]->IsExternal()) {     \
    return ThrowException(Exception::TypeError(             \
      String::New("Argument " #I " invalid")));             \
  }                                                         \
  Local<External> VAR = Local<External>::Cast(args[I]);

#define REQ_FUN_ARG(I, VAR)                                 \
  if (args.Length() <= (I) || !args[I]->IsFunction()) {     \
    return ThrowException(Exception::TypeError(             \
      String::New("Argument " #I " must be a function")));  \
  }                                                         \
  Local<Function> VAR = Local<Function>::Cast(args[I]);

#define REQ_OBJ_ARG(I, VAR)                                 \
  if (args.Length() <= (I) || !args[I]->IsObject()) {       \
    return ThrowException(Exception::TypeError(             \
      String::New("Argument " #I " must be an Object")));   \
  }                                                         \
  Local<Array> VAR = Local<Array>::Cast(args[I]);

#define REQ_ARR_ARG(I, VAR)                                 \
  if (args.Length() <= (I) || !args[I]->IsArray()) {        \
    return ThrowException(Exception::TypeError(             \
      String::New("Argument " #I " must be an Array")));    \
  }                                                         \
  Local<Array> VAR = Local<Array>::Cast(args[I]);

#define REQ_STR_ARG(I, VAR)                                 \
  if (args.Length() <= (I) || !args[I]->IsString()) {       \
    return ThrowException(Exception::TypeError(             \
      String::New("Argument " #I " must be a string")));    \
  }                                                         \
  String::Utf8Value VAR(args[I]->ToString());

#define REQ_INT_ARG(I, VAR)                                 \
  if (args.Length() <= (I) || !args[I]->IsInt32()) {        \
    return ThrowException(Exception::TypeError(             \
      String::New("Argument " #I " must be an integer")));  \
  }                                                         \
  int VAR = args[I]->Int32Value();

#define RETURN_ON_ERROR(REQ, FNAME, ARGS, CLEANUP)          \
  REQ->ret = FNAME ARGS;                                    \
  if (REQ->ret < GP_OK) {                                   \
    printf(#FNAME"=%d\n", REQ->ret);                        \
    CLEANUP;                                                \
    return;                                                 \
  }

#define V8STR(str) String::New(str)
#define V8STR2(str, len) String::New(str, len)

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
