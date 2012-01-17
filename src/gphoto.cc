#include "binding.h"
#include "gphoto.h"
#include "camera.h"

Persistent<FunctionTemplate> GPhoto2::constructor_template;

GPhoto2::GPhoto2() : portinfolist_(NULL), abilities_(NULL) {
  this->context_ = gp_context_new();
  gp_context_set_error_func (this->context_, onError, NULL);
  gp_context_set_status_func (this->context_, onStatus, NULL);
  gp_camera_new (&this->_camera);
}

GPhoto2::~GPhoto2(){
  printf("GPhoto2 destructor\n");
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
    
    eio_custom(EIO_List, EIO_PRI_DEFAULT, EIO_ListCb, list_req);
    ev_ref(EV_DEFAULT_UC);
    gphoto->Ref();
    return Undefined();
}


void GPhoto2::EIO_List(eio_req *req){  
    int i, ret;
    list_request *list_req = (list_request *)req->data;
    GPhoto2 *gphoto = list_req->gphoto;
    GPPortInfoList *portInfoList = gphoto->getPortInfoList();
    CameraAbilitiesList *abilitiesList = gphoto->getAbilitiesList();
    gp_list_new(&list_req->list);
    
    ret = autodetect(list_req->list,  gphoto->getContext(), &portInfoList, &abilitiesList);
    
    int count = gp_list_count(list_req->list);
    gphoto->setAbilitiesList(abilitiesList);
    gphoto->setPortInfoList(portInfoList);
}
int  GPhoto2::EIO_ListCb(eio_req *req){

    HandleScope scope;
    int i;
    ev_unref(EV_DEFAULT_UC);
    
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
      result->Set(Number::New(i), Persistent<Object>::New(js_camera));     		
      if (try_catch.HasCaught()) node::FatalException(try_catch);
      
    }
    
    TryCatch try_catch;
    
    list_req->cb->Call(Context::GetCurrent()->Global(), 1, argv);
    
    if (try_catch.HasCaught()) node::FatalException(try_catch);
    
    list_req->cb.Dispose();
    list_req->gphoto->Unref();
    gp_list_free(list_req->list);    
    delete list_req;
    return 0;
}

int GPhoto2::openCamera(GPCamera *p){
  this->Ref();
  printf("Opening camera %d context=%p\n", __LINE__, this->context_);
  Camera *camera;
  int ret = open_camera(&camera, p->getModel(), p->getPort(), this->portinfolist_, this->abilities_);
  p->setCamera(camera);
  return ret;
}

int GPhoto2::closeCamera(GPCamera *p){
  this->Unref();
  printf("Closing camera %s", p->getModel().c_str());
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