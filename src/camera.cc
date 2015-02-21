/* Copyright contributors of the node-gphoto2 project */

#include <gphoto2/gphoto2-widget.h>

#include <sstream>
#include <string>
#include "./camera.h"

v8::Persistent<v8::Function> GPCamera::constructor;

GPCamera::GPCamera(v8::Handle<v8::External> js_gphoto, std::string model, std::string port)
    : ObjectWrap(),
      model_(model),
      port_(port),
      camera_(NULL),
      config_(NULL) {
  NanScope();
  GPhoto2 *gphoto = static_cast<GPhoto2 *>(js_gphoto->Value());
  NanAssignPersistent(this->gphoto, js_gphoto);
  this->gphoto_ = gphoto;
  uv_mutex_init(&this->cameraMutex);
}

GPCamera::~GPCamera() {
  printf("Camera destructor\n");
  this->gphoto_->closeCamera(this);
  NanDisposePersistent(this->gphoto);
  this->close();

  uv_mutex_destroy(&this->cameraMutex);
}

void GPCamera::Initialize(v8::Handle<v8::Object> exports) {
  NanScope();

  v8::Local<v8::FunctionTemplate> tpl = NanNew<v8::FunctionTemplate>(New);
  tpl->InstanceTemplate()->SetInternalFieldCount(1);
  tpl->SetClassName(NanNew("Camera"));

  NODE_SET_PROTOTYPE_METHOD(tpl, "getConfig", GetConfig);
  NODE_SET_PROTOTYPE_METHOD(tpl, "setConfigValue", SetConfigValue);
  NODE_SET_PROTOTYPE_METHOD(tpl, "takePicture", TakePicture);
  NODE_SET_PROTOTYPE_METHOD(tpl, "downloadPicture", DownloadPicture);

  NanAssignPersistent(constructor, tpl->GetFunction());

  exports->Set(NanNew("Camera"), tpl->GetFunction());
}

NAN_METHOD(GPCamera::TakePicture) {
  NanScope();
  GPCamera *camera = ObjectWrap::Unwrap<GPCamera>(args.This());
  camera->Ref();
  take_picture_request *picture_req;

  if (args.Length() >= 2) {
    REQ_OBJ_ARG(0, options);
    REQ_FUN_ARG(1, cb);
    picture_req = new take_picture_request();
    picture_req->preview = false;
    NanAssignPersistent(picture_req->cb, cb);

    if (options->Get(NanNew("targetPath"))->IsString()) {
      picture_req->target_path = std::string(*NanAsciiString(options->Get(NanNew("targetPath"))));
      picture_req->download = true;
    }
    if (options->Get(NanNew("socket"))->IsString()) {
      picture_req->socket_path = std::string(*NanAsciiString(options->Get(NanNew("socket"))));
      picture_req->download = true;
    }
    if (options->Get(NanNew("download"))->IsBoolean()) {
      picture_req->download =
          options->Get(NanNew("download"))->ToBoolean()->Value();
    }
    if (options->Get(NanNew("preview"))->IsBoolean()) {
      picture_req->preview =
          options->Get(NanNew("preview"))->ToBoolean()->Value();
    }
  } else {
    REQ_FUN_ARG(0, cb);
    picture_req = new take_picture_request();
    picture_req->preview = false;
    NanAssignPersistent(picture_req->cb, cb);
  }

  picture_req->camera = camera->getCamera();
  picture_req->cameraObject = camera;

  picture_req->context = gp_context_new();
  DO_ASYNC(picture_req, Async_Capture, Async_CaptureCb);
  return NanUndefined();
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
  NanScope();
  take_picture_request *capture_req =
    static_cast<take_picture_request *>(req->data);

  v8::Handle<v8::Value> argv[2];
  int argc = 1;
  argv[0] = NanUndefined();
  if (capture_req->ret != GP_OK) {
    argv[0] = NanNew(capture_req->ret);
  } else if (capture_req->download && !capture_req->target_path.empty()) {
    argc = 2;
    argv[1] = NanNew(capture_req->target_path.c_str());
  } else if (capture_req->data && capture_req->download) {
    argc = 2;
    v8::Local<v8::Object> globalObj = v8::Context::GetCurrent()->Global();
    v8::Local<v8::Function> bufferConstructor = v8::Local<v8::Function>::Cast(globalObj->Get(NanNew("Buffer")));
    v8::Handle<v8::Value> constructorArgs[1];
    if (capture_req->length > 0) {
      constructorArgs[0] = NanNew((unsigned int)capture_req->length);
    } else {
      constructorArgs[0] = NanNew(0);
    }

    v8::Local<v8::Object> buffer = bufferConstructor->NewInstance(1, constructorArgs);
    if (capture_req->length) {
      memmove(node::Buffer::Data(buffer), capture_req->data, capture_req->length);
      delete capture_req->data;
    }
    argv[1] = buffer;
  } else {
    argc = 2;
    argv[1] = NanNew(capture_req->path);
  }

  capture_req->cb->Call(v8::Context::GetCurrent()->Global(), argc, argv);
  NanDisposePersistent(capture_req->cb);
  if (capture_req->ret == GP_OK) gp_file_free(capture_req->file);
  capture_req->cameraObject->Unref();
  gp_context_unref(capture_req->context);
  // gp_camera_unref(capture_req->camera);
  delete capture_req;
}


NAN_METHOD(GPCamera::DownloadPicture) {
  NanScope();

  REQ_OBJ_ARG(0, options);
  REQ_FUN_ARG(1, cb);

  GPCamera *camera = ObjectWrap::Unwrap<GPCamera>(args.This());
  camera->Ref();
  take_picture_request *picture_req = new take_picture_request();

  NanAssignPersistent(picture_req->cb, cb);
  picture_req->camera = camera->getCamera();
  picture_req->cameraObject = camera;
  picture_req->context = gp_context_new();
  picture_req->download = true;

  v8::Local<v8::Value> source = options->Get(NanNew("cameraPath"));
  v8::Local<v8::Value> target = options->Get(NanNew("targetPath"));
  if (target->IsString()) {
    picture_req->target_path = std::string(*NanAsciiString(target));
  }

  picture_req->path     = std::string(*NanAsciiString(source));
  gp_camera_ref(picture_req->camera);
  DO_ASYNC(picture_req, Async_DownloadPicture, Async_CaptureCb);
  return NanUndefined();
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

NAN_METHOD(GPCamera::GetConfig) {
  NanScope();
  REQ_FUN_ARG(0, cb)
  GPCamera *camera = ObjectWrap::Unwrap<GPCamera>(args.This());
  camera->Ref();
  get_config_request *config_req = new get_config_request();
  config_req->cameraObject = camera;
  config_req->camera = camera->getCamera();
  gp_camera_ref(config_req->camera);
  config_req->context = gp_context_new();
  NanAssignPersistent(config_req->cb, cb);

  DO_ASYNC(config_req, Async_GetConfig, Async_GetConfigCb);

  // Async_custom(Async_GetConfig, Async_PRI_DEFAULT, Async_GetConfigCb,
  //                config_req);
  // ev_ref(EV_DEFAULT_UC);
  return NanUndefined();
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
    ret = enumConfig(config_req, config_req->root, &config_req->settings);
    config_req->ret = ret;
  }
}

void GPCamera::Async_GetConfigCb(uv_work_t *req, int status) {
  NanScope();
  get_config_request *config_req = static_cast<get_config_request *>(req->data);

  v8::Handle<v8::Value> argv[2];

  if (config_req->ret == GP_OK) {
    argv[0] = NanUndefined();
    // TODO(lwille): properly cast config structure
    // argv[1] = cv::CastToJS(config_req->settings);

  } else {
    argv[0] = NanNew(config_req->ret);
    argv[1] = NanUndefined();
  }

  config_req->cb->Call(v8::Context::GetCurrent()->Global(), 2, argv);

  gp_widget_free(config_req->root);
  NanDisposePersistent(config_req->cb);
  config_req->cameraObject->Unref();
  gp_context_unref(config_req->context);
  gp_camera_unref(config_req->camera);

  delete config_req;
}

NAN_METHOD(GPCamera::SetConfigValue) {
  NanScope();
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
    return NanThrowTypeError("Argument 1 invalid: String, Integer or Float value expected");
  }
  config_req->cameraObject = camera;
  config_req->camera = camera->getCamera();
  config_req->context = gp_context_new();
  NanAssignPersistent(config_req->cb, cb);
  config_req->key = *key;

  gp_camera_ref(config_req->camera);
  DO_ASYNC(config_req, Async_SetConfigValue, Async_SetConfigValueCb);

  return NanUndefined();
}

void GPCamera::Async_SetConfigValue(uv_work_t *req) {
  set_config_request *config_req = static_cast<set_config_request *>(req->data);

  config_req->cameraObject->lock();
  config_req->ret = setWidgetValue(config_req);
  config_req->cameraObject->unlock();
}

void GPCamera::Async_SetConfigValueCb(uv_work_t *req, int status) {
  NanScope();
  set_config_request *config_req = static_cast<set_config_request *>(req->data);

  int argc = 0;
  v8::Local<v8::Value> argv[1];
  if (config_req->ret < GP_OK) {
    argv[0] = NanNew(config_req->ret);
    argc = 1;
  }
  config_req->cb->Call(v8::Context::GetCurrent()->Global(), argc, argv);
  NanDisposePersistent(config_req->cb);
  config_req->cameraObject->Unref();
  gp_context_unref(config_req->context);
  gp_camera_unref(config_req->camera);
  delete config_req;
}

NAN_METHOD(GPCamera::New) {
  NanScope();

  REQ_EXT_ARG(0, js_gphoto);
  REQ_STR_ARG(1, model_);
  REQ_STR_ARG(2, port_);

  GPCamera *camera = new GPCamera(js_gphoto, (std::string) *model_,
                                  (std::string) *port_);
  camera->Wrap(args.This());
  v8::Local<v8::Object> This = args.This();
  This->Set(NanNew("model"), NanNew(camera->model_.c_str()));
  This->Set(NanNew("port"), NanNew(camera->port_.c_str()));
  return args.This();
}

Camera* GPCamera::getCamera() {
  // printf("getCamera %s gphoto=%p\n", this->isOpen() ? "open" : "closed", gp);
  if (!this->isOpen()) {
    this->gphoto_->openCamera(this);
  }
  return this->camera_;
}

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
