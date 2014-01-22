/* Copyright 2012 Leonhardt Wille */

#include <string>
#include "camera.h"  // NOLINT
#include "gphoto.h"  // NOLINT

Persistent<FunctionTemplate> GPhoto2::constructor_template;

static void onError(GPContext *context, const char *str, void *data) {
  fprintf(stderr, "### %s\n", str);
}

static void onStatus(GPContext *context, const char *str, void *data) {
  fprintf(stderr, "### %s\n", str);
}

GPhoto2::GPhoto2() : portinfolist_(NULL), abilities_(NULL) {
  this->context_ = gp_context_new();
  gp_context_set_error_func(this->context_, onError, NULL);
  gp_context_set_status_func(this->context_, onStatus, NULL);
  gp_camera_new(&this->_camera);
}

GPhoto2::~GPhoto2() {
}

void GPhoto2::Initialize(Handle<Object> target) {
  HandleScope scope;
  Local<FunctionTemplate> t = FunctionTemplate::New(New);

  // Constructor
  constructor_template = Persistent<FunctionTemplate>::New(t);


  constructor_template->InstanceTemplate()->SetInternalFieldCount(1);
  constructor_template->SetClassName(String::NewSymbol("GPhoto2"));

  ADD_PROTOTYPE_METHOD(gphoto, test, Test);
  ADD_PROTOTYPE_METHOD(gphoto, list, List);
  ADD_PROTOTYPE_METHOD(gphoto, onLog, SetLogHandler);

  target->Set(String::NewSymbol("GPhoto2"),
              constructor_template->GetFunction());
}

Handle<Value> GPhoto2::New(const Arguments &args) {
  HandleScope scope;
  GPhoto2 *gphoto = new GPhoto2();
  gphoto->Wrap(args.This());
  return args.This();
}

Handle<Value> GPhoto2::Test(const Arguments &args) {
  printf("(Test)Everything is fine\n");
  return Undefined();
}

Handle<Value> GPhoto2::List(const Arguments &args) {
  HandleScope scope;

  REQ_FUN_ARG(0, cb);
  GPhoto2 *gphoto = ObjectWrap::Unwrap<GPhoto2>(args.This());

  TryCatch try_catch;
  list_request *list_req = new list_request();
  list_req->cb = Persistent<Function>::New(cb);
  list_req->list = NULL;
  list_req->gphoto = gphoto;
  list_req->This = Persistent<Object>::New(args.This());
  list_req->context = gp_context_new();

  DO_ASYNC(list_req, Async_List, Async_ListCb);

  gphoto->Ref();
  return Undefined();
}

void GPhoto2::Async_LogCallback(uv_async_t *handle, int status) {
  log_request *message = static_cast<log_request *>(handle->data);

  HandleScope scope;
  Local<Value> args[] = {
    Local<Value>::New(Number::New(message->level)),
    Local<Value>::New(String::New(message->domain.c_str())),
    Local<Value>::New(String::New(message->message.c_str()))
  };
  message->cb->Call(message->cb, 3, args);
  scope.Close(Undefined());
  delete message;
}

void GPhoto2::LogHandler(GPLogLevel level, const char *domain, const char *str,
                          void *data) {
  log_request *message = new log_request();
  static uv_async_t asyncLog;

  message->level = level;
  message->domain = std::string(domain);
  message->message = std::string(str);
  message->cb = static_cast<Function *>(data);
  uv_async_init(uv_default_loop(), &asyncLog, GPhoto2::Async_LogCallback);
  asyncLog.data = static_cast<void *>(message);

  uv_async_send(&asyncLog);
  sleep(0);  // allow the default thread to process the log entry
}

Handle<Value> GPhoto2::SetLogHandler(const Arguments &args) {
  HandleScope scope;
  REQ_ARGS(2);
  REQ_INT_ARG(0, level);
  REQ_FUN_ARG(1, cb);

  return cvv8::CastToJS(
    gp_log_add_func((GPLogLevel) level, (GPLogFunc) GPhoto2::LogHandler,
                    static_cast<void *>(*Persistent<Function>::New(cb))));
}

void GPhoto2::Async_List(uv_work_t *req) {
  list_request *list_req = static_cast<list_request *>(req->data);
  GPhoto2 *gphoto = list_req->gphoto;
  GPPortInfoList *portInfoList = gphoto->getPortInfoList();
  CameraAbilitiesList *abilitiesList = gphoto->getAbilitiesList();
  gp_list_new(&list_req->list);
  autodetect(list_req->list, list_req->context, &portInfoList, &abilitiesList);
  gphoto->setAbilitiesList(abilitiesList);
  gphoto->setPortInfoList(portInfoList);
}

void  GPhoto2::Async_ListCb(uv_work_t *req, int status) {
    HandleScope scope;
    int i;

    list_request *list_req = static_cast<list_request *>(req->data);

    Local<Value> argv[1];

    int count = gp_list_count(list_req->list);
    Local<Array> result = Array::New(count);
    argv[0] = result;
    for (i = 0; i < count; i++) {
      const char *name_, *port_;
      gp_list_get_name(list_req->list, i, &name_);
      gp_list_get_value(list_req->list, i, &port_);

      Local<Value> _argv[3];

      _argv[0] = External::New(list_req->gphoto);
      _argv[1] = String::New(name_);
      _argv[2] = String::New(port_);
      TryCatch try_catch;
      // call the _javascript_ constructor to create a new Camera object
      Persistent<Object> js_camera(
        GPCamera::constructor_template->GetFunction()->NewInstance(3, _argv));
      js_camera->Set(String::NewSymbol("_gphoto2_ref_obj_"), list_req->This);
      result->Set(Number::New(i), Persistent<Object>::New(js_camera));
      if (try_catch.HasCaught()) node::FatalException(try_catch);
    }

    TryCatch try_catch;

    list_req->cb->Call(Context::GetCurrent()->Global(), 1, argv);

    if (try_catch.HasCaught()) node::FatalException(try_catch);
    list_req->This.Dispose();
    list_req->cb.Dispose();
    list_req->gphoto->Unref();
    gp_context_unref(list_req->context);
    gp_list_free(list_req->list);
    delete list_req;
}

int GPhoto2::openCamera(GPCamera *p) {
  this->Ref();
  // printf("Opening camera %d context=%p\n", __LINE__, this->context_);
  Camera *camera;
  int ret = open_camera(&camera, p->getModel(), p->getPort(),
                        this->portinfolist_, this->abilities_);
  p->setCamera(camera);
  return ret;
}

int GPhoto2::closeCamera(GPCamera *p) {
  this->Unref();
  // printf("Closing camera %s", p->getModel().c_str());
  return 0;
}
