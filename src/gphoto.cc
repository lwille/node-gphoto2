#include "gphoto.h"
#include "camera.h"

#include <string.h>
Persistent<FunctionTemplate> GPhoto2::constructor_template;

GPhoto2::GPhoto2() : portinfolist_(NULL), abilities_(NULL) {
  this->context_ = gp_context_new();
  gp_context_set_error_func (this->context_, onError, NULL);
  gp_context_set_status_func (this->context_, onStatus, NULL);
  gp_camera_new (&this->_camera);
}

GPhoto2::~GPhoto2(){
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
    
  target->Set(String::NewSymbol("GPhoto2"), constructor_template->GetFunction());
}


    
Handle<Value> GPhoto2::New(const Arguments &args) {
    HandleScope scope;
    GPhoto2 *gphoto = new GPhoto2();
    gphoto->Wrap(args.This());
    return args.This();
}

Handle<Value> GPhoto2::Test(const Arguments &args){
  printf("(Test)Everything is fine\n");
  return Undefined();
}

Handle<Value> GPhoto2::List(const Arguments &args){
    HandleScope scope;
    
    REQ_FUN_ARG(0, cb);
    GPhoto2 *gphoto = ObjectWrap::Unwrap<GPhoto2>(args.This());
    
    TryCatch try_catch;
    list_request *list_req =new list_request();
    list_req->cb = Persistent<Function>::New(cb);
    list_req->list = NULL;
    list_req->gphoto = gphoto;
    list_req->This = Persistent<Object>::New(args.This());
    list_req->context  = gp_context_new();
    
    //eio_custom(EIO_List, EIO_PRI_DEFAULT, EIO_ListCb, list_req);
    //ev_ref(EV_DEFAULT_UC);
    DO_ASYNC(list_req, EIO_List, EIO_ListCb);
    
    gphoto->Ref();
    return Undefined();
}

void GPhoto2::LogHandler(GPLogLevel level, const char *domain, const char *format, va_list args, void *data){
  HandleScope scope;
  GPhoto2 *instance = (GPhoto2*)data;
  if(instance->logCallBack != Undefined()){
    
    Handle<Value> argv[] = {cvv8::CastToJS(domain), cvv8::CastToJS(format), cvv8::CastToJS(args)};
    instance->logCallBack->Call(Context::GetCurrent()->Global(), 3, argv);
  }
}

Handle<Value> GPhoto2::SetLogHandler(const Arguments &args){
  HandleScope scope;
  // REQ_INT_ARG(0, level);
  // REQ_FUN_ARG(1, cb);

  // GPhoto2 *gphoto = ObjectWrap::Unwrap<GPhoto2>(args.This());
  // gphoto->logCallBack = Persistent<Function>::New(cb);
  // gp_log_add_func((GPLogLevel)level, GPhoto2::LogHandler, gphoto);
  return Undefined();
}

void GPhoto2::EIO_List(uv_work_t *req){  
    int ret;
    list_request *list_req = (list_request *)req->data;
    GPhoto2 *gphoto = list_req->gphoto;
    GPPortInfoList *portInfoList = gphoto->getPortInfoList();
    CameraAbilitiesList *abilitiesList = gphoto->getAbilitiesList();
    gp_list_new(&list_req->list);
    ret = autodetect(list_req->list,  list_req->context, &portInfoList, &abilitiesList);
    gphoto->setAbilitiesList(abilitiesList);
    gphoto->setPortInfoList(portInfoList);
}
void  GPhoto2::EIO_ListCb(uv_work_t *req){

    HandleScope scope;
    int i;
  
    list_request *list_req = (list_request *)req->data;
    
    Local<Value> argv[1];
    
    int count = gp_list_count(list_req->list);
    Local<Array> result = Array::New(count);
    argv[0] = result;
    for(i=0; i<count; i++) {        
   		const char *name_, *port_;
   		gp_list_get_name (list_req->list, i, &name_);
   		gp_list_get_value (list_req->list, i, &port_);
   		
      Local<Value> constructor_args[3];
      
      constructor_args[0] = External::New(list_req->gphoto);
      constructor_args[1] = String::New(name_);
      constructor_args[2] = String::New(port_);
      TryCatch try_catch;
      // call the _javascript_ constructor to create a new Camera object
   		Persistent<Object> js_camera(GPCamera::constructor_template->GetFunction()->NewInstance(3, constructor_args));
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

int GPhoto2::openCamera(GPCamera *p){
  this->Ref();
  //printf("Opening camera %d context=%p\n", __LINE__, this->context_);
  Camera *camera;
  int ret = open_camera(&camera, p->getModel(), p->getPort(), this->portinfolist_, this->abilities_);
  p->setCamera(camera);
  return ret;
}

int GPhoto2::closeCamera(GPCamera *p){
  this->Unref();
  //printf("Closing camera %s", p->getModel().c_str());
  return 0;
}

#ifdef OLDLIB
void onError (GPContext *context, const char *format, va_list args, void *data) {
    fprintf  (stdout, "\n");
    fprintf  (stdout, "*** Contexterror ***              \n");
    vfprintf (stdout, format, args);
    fprintf  (stdout, "\n");
    fflush   (stdout);
}
void onStatus (GPContext *context, const char *format, va_list args, void *data) {
    vfprintf (stdout, format, args);
    fprintf  (stdout, "\n");
    fflush   (stdout);
}
#else
void
onError (GPContext *context, const char *str, void *data)
{
	printf ("### %s\n", str);
}

static void onStatus (GPContext *context, const char *str, void *data) {
	printf ("### %s\n", str);
}
#endif