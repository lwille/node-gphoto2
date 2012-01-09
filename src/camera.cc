#include "binding.h"
#include "camera.h"

using namespace v8;
using namespace node;


Persistent<FunctionTemplate> GPCamera::constructor_template;

GPCamera::GPCamera(GPhoto2* gphoto, std::string model, std::string port) : model_(model), port_(port), ObjectWrap(){}
GPCamera::~GPCamera(){
  this->close();
}
Handle<Value> GPCamera::Test(const Arguments &args){
  printf("Everything is fine");
}

void
GPCamera::Initialize(Handle<Object> target) {
    HandleScope scope;
    Local<FunctionTemplate> t = FunctionTemplate::New(New);
    
    // Constructor
    constructor_template = Persistent<FunctionTemplate>::New(t);
    
    
    constructor_template->InstanceTemplate()->SetInternalFieldCount(1);
    constructor_template->SetClassName(String::NewSymbol("Camera"));

    ADD_PROTOTYPE_METHOD(camera, test, Test);
    
    target->Set(String::NewSymbol("Camera"), constructor_template->GetFunction());
}


Handle<Value>
GPCamera::New(const Arguments& args) {
  HandleScope scope;
  
  REQ_EXT_ARG(0, js_gphoto);
  GPhoto2 *gphoto = static_cast<GPhoto2*>(js_gphoto->Value());

  REQ_STR_ARG(1, model_);
  REQ_STR_ARG(2, port_);

  GPCamera *camera = new GPCamera(gphoto, (std::string)*model_, (std::string)*port_);
  camera->Wrap(args.This());
  Local<Object> This = args.This();
  This->Set(String::New("model"),String::New(camera->model_.c_str()));
  This->Set(String::New("port"),String::New(camera->port_.c_str()));
  return args.This();
}  
bool
GPCamera::open(){
  return open_camera(&this->camera_, this->model_, this->port_) >= GP_OK;
}

bool
GPCamera::close(){
  // this->gphoto_->Unref();
  if(this->camera_)
    return gp_camera_exit(this->camera_, this->gphoto_->getContext()) < GP_OK ? false : true;
  else
    return true;    
}
