/* Copyright contributors of the node-gphoto2 project */

#include <string>

#include "./camera.h"
#include "./gphoto.h"

Nan::Persistent<v8::Function> GPhoto2::constructor;

static void onError(GPContext *context, const char *str, void *data) {
  fprintf(stderr, "### %s\n", str);
}

static void onStatus(GPContext *context, const char *str, void *data) {
  fprintf(stderr, "### %s\n", str);
}

GPhoto2::GPhoto2() : Nan::ObjectWrap(), portinfolist_(NULL), abilities_(NULL) {
  this->context_ = gp_context_new();

  gp_context_set_error_func(this->context_, onError, NULL);
  gp_context_set_status_func(this->context_, onStatus, NULL);
  gp_camera_new(&this->_camera);

  uv_mutex_init(&lstMutex);
  uv_mutex_init(&logMutex);

  asyncLog.data = reinterpret_cast<void*>(this);
  uv_async_init(uv_default_loop(), &asyncLog, GPhoto2::Async_LogCallback);
  uv_unref(reinterpret_cast<uv_handle_t*>(&asyncLog));
}

GPhoto2::~GPhoto2() {
  if (logFuncId) {
    gp_log_remove_func(logFuncId);
    logFuncId = 0;
  }

  if (emitFuncCb != NULL) {
    emitFuncCb->Reset();
    delete emitFuncCb;
    emitFuncCb = NULL;
  }
}

NAN_MODULE_INIT(GPhoto2::Initialize) {
  v8::Local<v8::FunctionTemplate> tpl = Nan::New<v8::FunctionTemplate>(New);

  tpl->InstanceTemplate()->SetInternalFieldCount(1);
  tpl->SetClassName(Nan::New("GPhoto2").ToLocalChecked());

  Nan::SetPrototypeMethod(tpl, "list", List);
  Nan::SetPrototypeMethod(tpl, "setLogLevel", SetLogLevel);

  constructor.Reset(Nan::GetFunction(tpl).ToLocalChecked());

  Nan::Set(target, Nan::New("GPhoto2").ToLocalChecked(), Nan::GetFunction(tpl).ToLocalChecked());
}

NAN_METHOD(GPhoto2::New) {
  GPhoto2 *gphoto = new GPhoto2();
  gphoto->Wrap(info.This());
  info.GetReturnValue().Set(info.This());

  v8::Local<v8::Value> emitFunc(Nan::Get(info.This(), Nan::New("emit").ToLocalChecked()).ToLocalChecked());
  gphoto->emitFuncCb = new Nan::Callback(v8::Local<v8::Function>::Cast(emitFunc));
}

NAN_METHOD(GPhoto2::List) {
  REQ_FUN_ARG(0, cb);

  GPhoto2 *gphoto = Nan::ObjectWrap::Unwrap<GPhoto2>(info.This());

  list_request *list_req = new list_request();
  list_req->cb.Reset(cb);
  list_req->list = NULL;
  list_req->gphoto = gphoto;
  list_req->This.Reset(info.This());
  list_req->context = gphoto->getContext();
  gp_context_ref(list_req->context);

  gphoto->Ref();
  DO_ASYNC(list_req, Async_List, Async_ListCb);
  info.GetReturnValue().SetUndefined();
}

NAN_METHOD(GPhoto2::SetLogLevel) {
  Nan::HandleScope scope;
  REQ_ARGS(1);

  GPhoto2 *gphoto = ObjectWrap::Unwrap<GPhoto2>(info.This());

  if (gphoto->logFuncId) {
    gp_log_remove_func(gphoto->logFuncId);
  }

  Nan::Maybe<int32_t> level = Nan::To<int32_t>(info[0]);
  if (level.IsNothing() || level.FromJust() < 0) {
    gphoto->logFuncId = 0;
    gphoto->logLevel = -1;
    return info.GetReturnValue().SetUndefined();
  }

  gphoto->logLevel = (GPLogLevel) level.FromJust();

  gp_log_add_func((GPLogLevel)gphoto->logLevel,
                  (GPLogFunc) GPhoto2::LogHandler,
                  static_cast<void *>(gphoto));

  return info.GetReturnValue().SetUndefined();
}

void GPhoto2::LogHandler(GPLogLevel level, const char *domain, const char *str, void *data) {
  GPhoto2 *gphoto = static_cast<GPhoto2*>(data);
  log_request *message = new log_request();

  message->level = level;
  message->domain = std::string(domain);
  message->message = std::string(str);

  uv_mutex_lock(&gphoto->logMutex);

  gphoto->logMessages.emplace_back(message);

  if (gphoto->logMessages.size() == 1) {
    uv_ref(reinterpret_cast<uv_handle_t*>(&gphoto->asyncLog));
    uv_async_send(&gphoto->asyncLog);
  }

  uv_mutex_unlock(&gphoto->logMutex);

  sleep(0);
}

NAUV_WORK_CB(GPhoto2::Async_LogCallback) {
  Nan::HandleScope scope;
  GPhoto2 *gphoto = static_cast<GPhoto2*>(async->data);

  uv_mutex_lock(&gphoto->logMutex);

  for (auto message : gphoto->logMessages) {
    v8::Local<v8::Value> args[] = {
      Nan::New("log").ToLocalChecked(),
      Nan::New(message->level),
      Nan::New(message->domain.c_str()).ToLocalChecked(),
      Nan::New(message->message.c_str()).ToLocalChecked()
    };
    Nan::Call(*gphoto->emitFuncCb, gphoto->handle(), 4, args);
    delete message;
  }

  gphoto->logMessages.clear();

  uv_unref(reinterpret_cast<uv_handle_t*>(&gphoto->asyncLog));
  uv_mutex_unlock(&gphoto->logMutex);
}

void GPhoto2::Async_List(uv_work_t *req) {
  list_request *list_req = static_cast<list_request *>(req->data);
  GPhoto2 *gphoto = list_req->gphoto;

  uv_mutex_lock(&gphoto->lstMutex);

  if (gphoto->abilities_ != NULL) {
    gp_abilities_list_free(gphoto->abilities_);
    gphoto->abilities_ = NULL;
  }

  gp_list_new(&list_req->list);
  autodetect(list_req->list, list_req->context, &gphoto->portinfolist_, &gphoto->abilities_);

  uv_mutex_unlock(&gphoto->lstMutex);
  sleep(0);
}

void GPhoto2::Async_ListCb(uv_work_t *req, int status) {
  Nan::HandleScope scope;
  int i;
  list_request *list_req = static_cast<list_request *>(req->data);
  Nan::TryCatch try_catch;
  v8::Local<v8::Value> argv[1];

  int count = gp_list_count(list_req->list);
  v8::Local<v8::Array> result = Nan::New<v8::Array>(count);
  argv[0] = result;

  for (i = 0; i < count; i++) {
    const char *name_, *port_;

    gp_list_get_name(list_req->list, i, &name_);
    gp_list_get_value(list_req->list, i, &port_);

    v8::Local<v8::Value> _argv[3];
    _argv[0] = Nan::New<v8::External>(list_req->gphoto);
    _argv[1] = Nan::New(name_).ToLocalChecked();
    _argv[2] = Nan::New(port_).ToLocalChecked();
    // call the _javascript_ constructor to create a new Camera object
    v8::Local<v8::Object> js_camera = Nan::NewInstance(Nan::New(GPCamera::constructor), 3, _argv).ToLocalChecked();

    Nan::Set(js_camera, Nan::New("_gphoto2_ref_obj_").ToLocalChecked(), Nan::New(list_req->This));
    Nan::Set(result, Nan::New(i), js_camera);
    if (try_catch.HasCaught()) {
      goto finally;
    }
  }

  Nan::Call(list_req->cb, 1, argv);

finally:
  gp_context_unref(list_req->context);
  gp_list_free(list_req->list);
  list_req->cb.Reset();
  list_req->This.Reset();
  list_req->gphoto->Unref();
  delete list_req;
  delete req;
  if (try_catch.HasCaught()) {
    return Nan::FatalException(try_catch);
  }
}

int GPhoto2::openCamera(GPCamera *p) {
  this->Ref();
  // printf("Opening camera %d context=%p\n", __LINE__, this->context_);
  Camera *camera;
  int ret;
  {
    uv_mutex_lock(&lstMutex);
    ret = open_camera(&camera, p->getModel(), p->getPort(), getPortInfoList(), getAbilitiesList());
    uv_mutex_unlock(&lstMutex);
  }
  p->setCamera(camera);

  return ret;
}

int GPhoto2::closeCamera(GPCamera *p) {
  this->Unref();
  // printf("Closing camera %s", p->getModel().c_str());
  return 0;
}
