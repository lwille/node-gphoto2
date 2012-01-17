#ifndef __BINDING_H
#define __BINDING_H

#define MALLOC_STRUCT(VAR, STRUCT) \
STRUCT *VAR = (STRUCT*)malloc(sizeof(STRUCT));
#define ADD_PROTOTYPE_METHOD(class, name, method) \
class ## _ ## name ## _symbol = NODE_PSYMBOL(#name); \
NODE_SET_PROTOTYPE_METHOD(constructor_template, #name, method);

#define REQ_EXT_ARG(I, VAR) \
if (args.Length() <= (I) || !args[I]->IsExternal()) \
return ThrowException(Exception::TypeError( \
String::New("Argument " #I " invalid"))); \
Local<External> VAR = Local<External>::Cast(args[I]);

#define REQ_FUN_ARG(I, VAR) \
if (args.Length() <= (I) || !args[I]->IsFunction()) \
return ThrowException(Exception::TypeError( \
String::New("Argument " #I " must be a function"))); \
Local<Function> VAR = Local<Function>::Cast(args[I]);


#define REQ_STR_ARG(I, VAR) \
if (args.Length() <= (I) || !args[I]->IsString()) \
return ThrowException(Exception::TypeError( \
String::New("Argument " #I " must be a string"))); \
String::Utf8Value VAR(args[I]->ToString());


#define RETURN_ON_ERROR(REQ, FNAME, ARGS, CLEANUP)\
REQ->ret = FNAME ARGS;\
if(REQ->ret < GP_OK){\
  printf(#FNAME"=%d\n", REQ->ret);\
  CLEANUP;\
  return;\
}



#define V8STR(str) String::New(str)
#define V8STR2(str, len) String::New(str, len)

#include <string>
#include <gphoto2/gphoto2-camera.h>

// Useful functions taken from library examples, with slight modifications
int open_camera (Camera ** camera, std::string model, std::string port, GPPortInfoList *portinfolist, CameraAbilitiesList	*abilities);
int  autodetect (CameraList *list, GPContext *context, GPPortInfoList **portinfolist, CameraAbilitiesList	**abilities);
int  set_config_value_string (Camera *camera, const char *key, const char *val, GPContext *context);
int  get_config_value_string (Camera *camera, const char *key, char **str, GPContext *context);
void capture_to_memory(Camera *camera, GPContext *context, const char **ptr, unsigned long int *size);


#endif