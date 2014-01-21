/* Copyright 2012 Leonhardt Wille */

#include <gphoto2/gphoto2-widget.h>
#include <sstream>
#include <string>
#include "camera.h"  // NOLINT 

using std::string;
using namespace node;  // NOLINT
using namespace v8;  // NOLINT

Persistent<FunctionTemplate> GPCamera::constructor_template;

GPCamera::GPCamera(Handle<External> js_gphoto, string model, string port)
    : ObjectWrap(),
      model_(model),
      port_(port),
      camera_(NULL),
      config_(NULL) {
  HandleScope scope;
  GPhoto2 *gphoto = static_cast<GPhoto2 *>(js_gphoto->Value());
  this->gphoto = Persistent<External>::New(js_gphoto);
  this->gphoto_ = gphoto;
  uv_mutex_init(&this->cameraMutex);
}

GPCamera::~GPCamera() {
  printf("Camera destructor\n");
  this->gphoto_->closeCamera(this);
  this->gphoto.Dispose();
  this->close();

  uv_mutex_destroy(&this->cameraMutex);
}

void GPCamera::Initialize(Handle<Object> target) {
  HandleScope scope;
  Local<FunctionTemplate> t = FunctionTemplate::New(New);

  // Constructor
  constructor_template = Persistent<FunctionTemplate>::New(t);
  constructor_template->InstanceTemplate()->SetInternalFieldCount(1);
  constructor_template->SetClassName(String::NewSymbol("Camera"));

  ADD_PROTOTYPE_METHOD(camera, getConfig, GetConfig);
  ADD_PROTOTYPE_METHOD(camera, setConfigValue, SetConfigValue);
  ADD_PROTOTYPE_METHOD(camera, takePicture, TakePicture);
  ADD_PROTOTYPE_METHOD(camera, downloadPicture, DownloadPicture);
  target->Set(String::NewSymbol("Camera"), constructor_template->GetFunction());
}

Handle<Value> GPCamera::TakePicture(const Arguments& args) {
  HandleScope scope;
  GPCamera *camera = ObjectWrap::Unwrap<GPCamera>(args.This());
  camera->Ref();
  take_picture_request *picture_req;

  if (args.Length() >= 2) {
    REQ_OBJ_ARG(0, options);
    REQ_FUN_ARG(1, cb);
    picture_req = new take_picture_request();
    picture_req->preview = false;
    picture_req->cb = Persistent<Function>::New(cb);
    Local<Value> dl = options->Get(String::New("download"));
    Local<Value> target = options->Get(String::New("targetPath"));
    Local<Value> preview = options->Get(String::New("preview"));
    Local<Value> socket  = options->Get(String::New("socket"));
    if (target->IsString()) {
      picture_req->target_path = cv::CastFromJS<string>(target);
      picture_req->download = true;
    }
    if (socket->IsString()) {
      picture_req->socket_path = cv::CastFromJS<string>(socket);
      picture_req->download = true;
    }
    if (dl->IsBoolean()) {
      picture_req->download = dl->ToBoolean()->Value();
    }
    if (preview->IsBoolean()) {
      picture_req->preview = preview->ToBoolean()->Value();
    }
  } else {
    REQ_FUN_ARG(0, cb);
    picture_req = new take_picture_request();
    picture_req->preview = false;
    picture_req->cb = Persistent<Function>::New(cb);
  }

  picture_req->camera = camera->getCamera();
  picture_req->cameraObject = camera;

  picture_req->context = gp_context_new();
  DO_ASYNC(picture_req, Async_Capture, Async_CaptureCb);
  return Undefined();
}

void GPCamera::Async_Capture(uv_work_t *_req) {
  take_picture_request *req = static_cast<take_picture_request *>(_req->data);
  req->cameraObject->lock();
  if (req->preview) {
    capturePreview(req);
  } else {
    takePicture(req);
  }
  req->cameraObject->unlock();
}

void GPCamera::Async_CaptureCb(uv_work_t *req, int status) {
  HandleScope scope;
  take_picture_request *capture_req =
    static_cast<take_picture_request *>(req->data);

  Handle<Value> argv[2];
  int argc = 1;
  argv[0] = Undefined();
  if (capture_req->ret != GP_OK) {
    argv[0] = Integer::New(capture_req->ret);
  } else if (capture_req->download && !capture_req->target_path.empty()) {
    argc = 2;
    argv[1] = String::New(capture_req->target_path.c_str());
  } else if (capture_req->data && capture_req->download) {
    argc = 2;
    Local<Object> globalObj = Context::GetCurrent()->Global();
    Local<Function> bufferConstructor =
      Local<Function>::Cast(globalObj->Get(String::New("Buffer")));
    Handle<Value> constructorArgs[1];
    constructorArgs[0] = capture_req->length
      ? Integer::New(capture_req->length)
      : Integer::New(0);
    Local<Object> buffer = bufferConstructor->NewInstance(1, constructorArgs);
    if (capture_req->length) {
      memmove(Buffer::Data(buffer), capture_req->data, capture_req->length);
      delete capture_req->data;
    }
    argv[1] = buffer;
  } else {
    argc = 2;
    argv[1] = cv::CastToJS(capture_req->path);
  }

  capture_req->cb->Call(Context::GetCurrent()->Global(), argc, argv);
  capture_req->cb.Dispose();
  if (capture_req->ret == GP_OK) gp_file_free(capture_req->file);
  capture_req->cameraObject->Unref();
  gp_context_unref(capture_req->context);
  // gp_camera_unref(capture_req->camera);
  delete capture_req;
}


Handle<Value> GPCamera::DownloadPicture(const Arguments& args) {
  HandleScope scope;

  REQ_OBJ_ARG(0, options);
  REQ_FUN_ARG(1, cb);

  GPCamera *camera = ObjectWrap::Unwrap<GPCamera>(args.This());
  camera->Ref();
  take_picture_request *picture_req = new take_picture_request();

  picture_req->cb = Persistent<Function>::New(cb);
  picture_req->camera = camera->getCamera();
  picture_req->cameraObject = camera;
  picture_req->context = gp_context_new();
  picture_req->download = true;

  Local<Value> source = options->Get(String::New("cameraPath"));
  Local<Value> target = options->Get(String::New("targetPath"));
  if (target->IsString()) {
    picture_req->target_path = cv::CastFromJS<string>(target);
  }

  picture_req->path     = cv::CastFromJS<string>(source);
  gp_camera_ref(picture_req->camera);
  DO_ASYNC(picture_req, Async_DownloadPicture, Async_CaptureCb);
  return Undefined();
}

void GPCamera::Async_DownloadPicture(uv_work_t *_req) {
  take_picture_request *req = static_cast<take_picture_request *>(_req->data);

  req->cameraObject->lock();
  downloadPicture(req);
  req->cameraObject->unlock();
}

/**
 * Return available configuration widgets as a list in the form
 * /main/status/model
 * /main/status/serialnumber
 * etc.
 */

Handle<Value> GPCamera::GetConfig(const Arguments& args) {
  HandleScope scope;
  REQ_FUN_ARG(0, cb)
  GPCamera *camera = ObjectWrap::Unwrap<GPCamera>(args.This());
  camera->Ref();
  get_config_request *config_req = new get_config_request();
  config_req->cameraObject = camera;
  config_req->camera = camera->getCamera();
  gp_camera_ref(config_req->camera);
  config_req->context = gp_context_new();
  config_req->cb = Persistent<Function>::New(cb);

  DO_ASYNC(config_req, Async_GetConfig, Async_GetConfigCb);

  // Async_custom(Async_GetConfig, Async_PRI_DEFAULT, Async_GetConfigCb,
  //                config_req);
  // ev_ref(EV_DEFAULT_UC);
  return Undefined();
}

namespace cvv8 {
  template<> struct NativeToJS<TreeNode> {
    v8::Handle<v8::Value> operator()(TreeNode const &node) const {
      HandleScope scope;
      if (node.value) {
        Handle<Value> value = GPCamera::getWidgetValue(node.context,
                                                       node.value);
        if (value->IsObject()) {
          Local<Object> obj = value->ToObject();
          if (node.subtree.size()) {
            obj->Set(cvv8::CastToJS("children"), cvv8::CastToJS(node.subtree));
          } else {
            obj->Set(cvv8::CastToJS("children"), Undefined());
          }
        }
        return scope.Close(value);
      } else {
        return scope.Close(Undefined());
      }
    }
  };
}

void GPCamera::Async_GetConfig(uv_work_t *req) {
  get_config_request *config_req = static_cast<get_config_request *>(req->data);
  int ret;

  config_req->cameraObject->lock();
  ret = gp_camera_get_config(config_req->camera, &config_req->root,
                             config_req->context);
  config_req->cameraObject->unlock();

  if (ret < GP_OK) {
    config_req->ret = ret;
  } else {
    ret = enumConfig(config_req, config_req->root, config_req->settings);
    config_req->ret = ret;
  }
}

void GPCamera::Async_GetConfigCb(uv_work_t *req, int status) {
  HandleScope scope;
  get_config_request *config_req = static_cast<get_config_request *>(req->data);

  Handle<Value> argv[2];

  if (config_req->ret == GP_OK) {
    argv[0] = Undefined();
    argv[1] = cv::CastToJS(config_req->settings);
  } else {
    argv[0] = cv::CastToJS(config_req->ret);
    argv[1] = Undefined();
  }

  config_req->cb->Call(Context::GetCurrent()->Global(), 2, argv);

  gp_widget_free(config_req->root);
  config_req->cb.Dispose();
  config_req->cameraObject->Unref();
  gp_context_unref(config_req->context);
  gp_camera_unref(config_req->camera);

  delete config_req;
}

Handle<Value> GPCamera::SetConfigValue(const Arguments &args) {
  HandleScope scope;
  GPCamera *camera = ObjectWrap::Unwrap<GPCamera>(args.This());
  camera->Ref();

  REQ_ARGS(3);
  REQ_STR_ARG(0, key);
  REQ_FUN_ARG(2, cb);

  set_config_request *config_req;
  if (args[1]->IsString()) {
    REQ_STR_ARG(1, value);
    config_req = new set_config_request();
    config_req->strValue = *value;
    config_req->valueType = set_config_request::String;
  } else if (args[1]->IsInt32()) {
    REQ_INT_ARG(1, value);
    config_req = new set_config_request();
    config_req->intValue = value;
    config_req->valueType = set_config_request::Integer;
  } else if (args[1]->IsNumber()) {
    double dblValue = args[1]->ToNumber()->Value();
    config_req = new set_config_request();
    config_req->fltValue = dblValue;
    config_req->valueType = set_config_request::Float;
  } else {
    return ThrowException(Exception::TypeError(
      String::New("Argument 1 invalid: String, Integer or Float value "
                  "expected")));
  }
  config_req->cameraObject = camera;
  config_req->camera = camera->getCamera();
  config_req->context = gp_context_new();
  config_req->cb = Persistent<Function>::New(cb);
  config_req->key = *key;

  gp_camera_ref(config_req->camera);
  DO_ASYNC(config_req, Async_SetConfigValue, Async_SetConfigValueCb);

  return Undefined();
}

void GPCamera::Async_SetConfigValue(uv_work_t *req) {
  set_config_request *config_req = static_cast<set_config_request *>(req->data);

  config_req->cameraObject->lock();
  config_req->ret = setWidgetValue(config_req);
  config_req->cameraObject->unlock();
}

void GPCamera::Async_SetConfigValueCb(uv_work_t *req, int status) {
  HandleScope scope;
  set_config_request *config_req = static_cast<set_config_request *>(req->data);

  int argc = 0;
  Local<Value> argv[1];
  if (config_req->ret < GP_OK) {
    argv[0] = Integer::New(config_req->ret);
    argc = 1;
  }
  config_req->cb->Call(Context::GetCurrent()->Global(), argc, argv);
  config_req->cb.Dispose();
  config_req->cameraObject->Unref();
  gp_context_unref(config_req->context);
  gp_camera_unref(config_req->camera);
  delete config_req;
}

Handle<Value> GPCamera::New(const Arguments& args) {
  HandleScope scope;

  REQ_EXT_ARG(0, js_gphoto);
  REQ_STR_ARG(1, model_);
  REQ_STR_ARG(2, port_);

  GPCamera *camera = new GPCamera(js_gphoto, (string) *model_,
                                  (string) *port_);
  camera->Wrap(args.This());
  Local<Object> This = args.This();
  This->Set(String::New("model"), String::New(camera->model_.c_str()));
  This->Set(String::New("port"), String::New(camera->port_.c_str()));
  return args.This();
}

Camera* GPCamera::getCamera() {
  // printf("getCamera %s gphoto=%p\n", this->isOpen() ? "open" : "closed", gp);
  if (!this->isOpen()) {
    this->gphoto_->openCamera(this);
  }
  return this->camera_;
};

bool GPCamera::close() {
  // this->gphoto_->Unref();
  if (this->camera_) {
    return gp_camera_exit(this->camera_, this->gphoto_->getContext()) < GP_OK
      ? false
      : true;
  } else {
    return true;
  }
}
