/* Copyright contributors of the node-gphoto2 project */

#include <gphoto2/gphoto2-widget.h>

#include <sstream>
#include <string>

#include "./camera.h"

Nan::Persistent<v8::Function> GPCamera::constructor;

GPCamera::GPCamera(v8::Local<v8::External> js_gphoto, std::string model, std::string port)
    : Nan::ObjectWrap(),
      model_(model),
      port_(port),
      camera_(NULL),
      config_(NULL) {
  Nan::HandleScope();
  GPhoto2 *gphoto = static_cast<GPhoto2 *>(js_gphoto->Value());
  this->gphoto.Reset(js_gphoto);
  this->gphoto_ = gphoto;
  uv_mutex_init(&this->cameraMutex);
}

GPCamera::~GPCamera() {
  this->gphoto_->closeCamera(this);
  this->close();

  uv_mutex_destroy(&this->cameraMutex);
}

NAN_MODULE_INIT(GPCamera::Initialize) {
  v8::Local<v8::FunctionTemplate> tpl = Nan::New<v8::FunctionTemplate>(New);
  tpl->InstanceTemplate()->SetInternalFieldCount(1);

  tpl->SetClassName(Nan::New("Camera").ToLocalChecked());

  Nan::SetPrototypeMethod(tpl, "getConfig", GetConfig);
  Nan::SetPrototypeMethod(tpl, "setConfigValue", SetConfigValue);
  Nan::SetPrototypeMethod(tpl, "takePicture", TakePicture);
  Nan::SetPrototypeMethod(tpl, "downloadPicture", DownloadPicture);

  constructor.Reset(Nan::GetFunction(tpl).ToLocalChecked());

  Nan::Set(target, Nan::New("Camera").ToLocalChecked(), Nan::GetFunction(tpl).ToLocalChecked());
}

NAN_METHOD(GPCamera::TakePicture) {
  GPCamera *camera = Nan::ObjectWrap::Unwrap<GPCamera>(info.Holder());
  camera->Ref();
  take_picture_request *picture_req;

  if (info.Length() >= 2) {
    REQ_OBJ_ARG(0, options);
    REQ_FUN_ARG(1, cb);

    picture_req = new take_picture_request();
    picture_req->file = NULL;
    picture_req->preview = false;
    picture_req->cb.Reset(cb);

    Nan::MaybeLocal<v8::String> targVal = MaybeGetLocal<v8::String>(options, "targetPath");
    if (!targVal.IsEmpty()) {
      Nan::Utf8String targetPath(targVal.ToLocalChecked());
      picture_req->target_path = std::string(*targetPath);
      picture_req->download = true;
    }

    Nan::MaybeLocal<v8::String> sktVal = MaybeGetLocal<v8::String>(options, "socket");
    if (!sktVal.IsEmpty()) {
      Nan::Utf8String socketPath(sktVal.ToLocalChecked());
      picture_req->socket_path = std::string(*socketPath);
      picture_req->download = true;
    }

    Nan::Maybe<bool> dlVal = MaybeGetValue<bool>(options, "download");
    if (!dlVal.IsNothing()) {
      picture_req->download = dlVal.FromJust();
    }

    Nan::Maybe<bool> preVal = MaybeGetValue<bool>(options, "preview");
    if (!preVal.IsNothing()) {
      picture_req->preview = preVal.FromJust();
    }

    Nan::Maybe<bool> keepVal = MaybeGetValue<bool>(options, "keep");
    if (!keepVal.IsNothing()) {
      picture_req->keep = keepVal.FromJust();
    }
  } else {
    REQ_FUN_ARG(0, cb);
    picture_req = new take_picture_request();
    picture_req->preview = false;
    picture_req->cb.Reset(cb);
    picture_req->file = NULL;
  }

  picture_req->camera = camera->getCamera();
  picture_req->cameraObject = camera;

  picture_req->context = camera->gphoto_->getContext();
  gp_context_ref(picture_req->context);

  DO_ASYNC(picture_req, Async_Capture, Async_CaptureCb);
  info.GetReturnValue().SetUndefined();
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

static void DeleteArray(char *data, void *hint) {
  delete [] data;
}

void GPCamera::Async_CaptureCb(uv_work_t *req, int status) {
  Nan::HandleScope scope;
  take_picture_request *capture_req = static_cast<take_picture_request *>(req->data);

  v8::Local<v8::Value> argv[2];
  int argc = 1;
  argv[0] = Nan::Undefined();

  if (capture_req->ret != GP_OK) {
    argv[0] = Nan::New(capture_req->ret);
  } else if (capture_req->download && !capture_req->target_path.empty()) {
    argc = 2;
    argv[1] = Nan::New(capture_req->target_path).ToLocalChecked();
  } else if (capture_req->data && capture_req->download) {
    argc = 2;
    if (capture_req->length) {
      argv[1] = Nan::NewBuffer(capture_req->data, capture_req->length, DeleteArray, nullptr).ToLocalChecked();
    } else {
      argv[1] = Nan::NewBuffer(0).ToLocalChecked();
    }
  } else {
    argc = 2;
    argv[1] = Nan::New(capture_req->path).ToLocalChecked();
  }

  Nan::Call(capture_req->cb, argc, argv);

  if (capture_req->ret == GP_OK && capture_req->file != NULL) {
    gp_file_free(capture_req->file);
  }
  capture_req->cameraObject->Unref();
  gp_context_unref(capture_req->context);
  capture_req->cb.Reset();
  delete capture_req;
}

NAN_METHOD(GPCamera::DownloadPicture) {
  REQ_OBJ_ARG(0, options);
  REQ_FUN_ARG(1, cb);

  GPCamera *camera = Nan::ObjectWrap::Unwrap<GPCamera>(info.This());
  camera->Ref();
  take_picture_request *picture_req = new take_picture_request();
  picture_req->cb.Reset(cb);
  picture_req->camera = camera->getCamera();
  picture_req->cameraObject = camera;
  picture_req->context = camera->gphoto_->getContext();
  gp_context_ref(picture_req->context);
  picture_req->download = true;

  v8::Local<v8::Value> source = Nan::Get(options, Nan::New("cameraPath").ToLocalChecked()).ToLocalChecked();
  v8::Local<v8::Value> target = Nan::Get(options, Nan::New("targetPath").ToLocalChecked()).ToLocalChecked();
  if (target->IsString()) {
    picture_req->target_path = std::string(*Nan::Utf8String(target));
  }

  picture_req->path = std::string(*Nan::Utf8String(source));
  gp_camera_ref(picture_req->camera);
  DO_ASYNC(picture_req, Async_DownloadPicture, Async_CaptureCb);
  info.GetReturnValue().SetUndefined();
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
  Nan::HandleScope scope;

  v8::Local<v8::Function> cb;
  bool simplify = false;
  const char *errMsg = "Usage: getConfig([optional bool - simpleStructure], callback)";

  switch (info.Length()) {
  case 0:
    return Nan::ThrowTypeError(errMsg);

  case 1:
    if (!info[0]->IsFunction()) {
      return Nan::ThrowTypeError(errMsg);
    }

    cb = v8::Local<v8::Function>::Cast(info[0]);
    break;

  default:
    simplify = Nan::To<v8::Boolean>(info[0]).ToLocalChecked()->Value();
    cb = v8::Local<v8::Function>::Cast(info[1]);
    break;
  }

  GPCamera *camera = ObjectWrap::Unwrap<GPCamera>(info.This());
  camera->Ref();

  get_config_request *config_req = new get_config_request();

  config_req->simpleSettings = simplify;
  config_req->cameraObject = camera;
  config_req->camera = camera->getCamera();
  gp_camera_ref(config_req->camera);
  config_req->context = camera->gphoto_->getContext();
  gp_context_ref(config_req->context);

  config_req->cb.Reset(cb);
  DO_ASYNC(config_req, Async_GetConfig, Async_GetConfigCb);

  info.GetReturnValue().SetUndefined();
}

void GPCamera::Async_GetConfig(uv_work_t *req) {
  get_config_request *config_req = static_cast<get_config_request *>(req->data);
  int ret;

  config_req->cameraObject->lock();
  ret = gp_camera_get_config(config_req->camera, &config_req->root, config_req->context);
  config_req->cameraObject->unlock();

  if (ret < GP_OK) {
    config_req->ret = ret;
  } else {
    ret = enumConfig(config_req, config_req->root, &config_req->settings);
    config_req->ret = ret;
  }
}

void GPCamera::Async_GetConfigCb(uv_work_t *req, int status) {
  Nan::HandleScope scope;
  get_config_request *config_req = static_cast<get_config_request *>(req->data);

  v8::Local<v8::Value> argv[2];

  if (config_req->ret == GP_OK) {
    argv[0] = Nan::Undefined();
    argv[1] = convertSettingsToObject(config_req->simpleSettings, config_req->context, config_req->settings);
  } else {
    argv[0] = Nan::New(config_req->ret);
    argv[1] = Nan::Undefined();
  }

  Nan::Call(config_req->cb, 2, argv);
  gp_widget_free(config_req->root);
  config_req->cameraObject->Unref();
  gp_context_unref(config_req->context);
  gp_camera_unref(config_req->camera);
  config_req->cb.Reset();
  delete config_req;
}

NAN_METHOD(GPCamera::SetConfigValue) {
  GPCamera *camera = ObjectWrap::Unwrap<GPCamera>(info.This());
  camera->Ref();

  REQ_ARGS(3);
  REQ_STR_ARG(0, key);
  REQ_FUN_ARG(2, cb);

  set_config_request *config_req;
  if (info[1]->IsString()) {
    REQ_STR_ARG(1, value);
    config_req = new set_config_request();
    config_req->strValue = *value;
    config_req->valueType = set_config_request::String;
  } else if (info[1]->IsInt32()) {
    REQ_INT_ARG(1, value);
    config_req = new set_config_request();
    config_req->intValue = value;
    config_req->valueType = set_config_request::Integer;
  } else if (info[1]->IsNumber()) {
    double dblValue = Nan::To<v8::Number>(info[1]).ToLocalChecked()->Value();
    config_req = new set_config_request();
    config_req->fltValue = dblValue;
    config_req->valueType = set_config_request::Float;
  } else {
    camera->Unref();
    return Nan::ThrowTypeError("Argument 1 invalid: String, Integer or Float value expected");
  }

  config_req->cameraObject = camera;
  config_req->camera = camera->getCamera();
  config_req->context = camera->gphoto_->getContext();
  gp_context_ref(config_req->context);
  config_req->cb.Reset(cb);
  config_req->key = *key;

  gp_camera_ref(config_req->camera);
  DO_ASYNC(config_req, Async_SetConfigValue, Async_SetConfigValueCb);

  info.GetReturnValue().SetUndefined();
}

void GPCamera::Async_SetConfigValue(uv_work_t *req) {
  set_config_request *config_req = static_cast<set_config_request *>(req->data);

  config_req->cameraObject->lock();
  config_req->ret = setWidgetValue(config_req);
  config_req->cameraObject->unlock();
}

void GPCamera::Async_SetConfigValueCb(uv_work_t *req, int status) {
  Nan::HandleScope scope;
  set_config_request *config_req = static_cast<set_config_request *>(req->data);

  int argc = 0;
  v8::Local<v8::Value> argv[1];
  if (config_req->ret < GP_OK) {
    argv[0] = Nan::New(config_req->ret);
    argc = 1;
  }

  Nan::Call(config_req->cb, argc, argv);

  config_req->cameraObject->Unref();
  gp_context_unref(config_req->context);
  gp_camera_unref(config_req->camera);
  config_req->cb.Reset();
  delete config_req;
}

NAN_METHOD(GPCamera::New) {
  REQ_EXT_ARG(0, js_gphoto);
  REQ_STR_ARG(1, model_);
  REQ_STR_ARG(2, port_);

  GPCamera *camera = new GPCamera(js_gphoto, (std::string) *model_, (std::string) *port_);

  camera->Wrap(info.This());
  Nan::Set(info.This(), Nan::New("model").ToLocalChecked(), Nan::New(camera->model_.c_str()).ToLocalChecked());
  Nan::Set(info.This(), Nan::New("port").ToLocalChecked(), Nan::New(camera->port_.c_str()).ToLocalChecked());
  info.GetReturnValue().Set(info.This());
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
