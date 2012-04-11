#include "camera.h"
#include <sstream>
#include <gphoto2/gphoto2-widget.h>

using namespace v8;
using namespace node;


Persistent<FunctionTemplate> GPCamera::constructor_template;

GPCamera::GPCamera(Handle<External> js_gphoto, std::string model, std::string port) : ObjectWrap(), model_(model), port_(port), camera_(NULL), config_(NULL){
  HandleScope scope;
  GPhoto2 *gphoto = static_cast<GPhoto2*>(js_gphoto->Value());
  this->gphoto = Persistent<External>::New(js_gphoto);
  this->gphoto_ = gphoto;
  pthread_mutex_init(&this->cameraMutex, NULL);
}
GPCamera::~GPCamera(){
  printf("Camera destructor\n");
  this->gphoto_->closeCamera(this);
  this->gphoto.Dispose();
  this->close();
  
  pthread_mutex_destroy(&this->cameraMutex);
}

void
GPCamera::Initialize(Handle<Object> target) {
    HandleScope scope;
    Local<FunctionTemplate> t = FunctionTemplate::New(New);
    
    // Constructor
    constructor_template = Persistent<FunctionTemplate>::New(t);
    
    
    constructor_template->InstanceTemplate()->SetInternalFieldCount(1);
    constructor_template->SetClassName(String::NewSymbol("Camera"));

    ADD_PROTOTYPE_METHOD(camera, getConfig, GetConfig);
    ADD_PROTOTYPE_METHOD(camera, setConfigValue, SetConfigValue);
    ADD_PROTOTYPE_METHOD(camera, takePicture, TakePicture);
    ADD_PROTOTYPE_METHOD(camera, getPreview, GetPreview);
    ADD_PROTOTYPE_METHOD(camera, downloadPicture, DownloadPicture);
    target->Set(String::NewSymbol("Camera"), constructor_template->GetFunction());
}

Handle<Value>
GPCamera::TakePicture(const Arguments& args) {
  HandleScope scope;
  printf("TakePicture %d\n", __LINE__);
  GPCamera *camera = ObjectWrap::Unwrap<GPCamera>(args.This());
  camera->Ref();
  take_picture_request *picture_req = new take_picture_request();
  if(args.Length() >= 2){
    REQ_OBJ_ARG(0, options);
    REQ_FUN_ARG(1, cb);    
    picture_req->cb = Persistent<Function>::New(cb);
    Local<Value> dl = options->Get(String::New("download"));
    if(dl->IsBoolean()){
      picture_req->download = dl->ToBoolean()->Value();
    }
  }else{
    REQ_FUN_ARG(0, cb);
    picture_req->cb = Persistent<Function>::New(cb);    
  }

  picture_req->camera = camera->getCamera();
  picture_req->cameraObject = camera;
  picture_req->context = gp_context_new();
  DO_ASYNC(picture_req, EIO_TakePicture, EIO_CapturePreviewCb);
  // eio_custom(EIO_TakePicture, EIO_PRI_DEFAULT, EIO_TakePictureCb, picture_req);
  // ev_ref(EV_DEFAULT_UC);
  return Undefined();
}
void
GPCamera::EIO_TakePicture(uv_work_t *_req){
  take_picture_request *req = (take_picture_request *)_req->data;

  req->cameraObject->lock();
  takePicture(req);
  req->cameraObject->unlock();
}

Handle<Value>
GPCamera::DownloadPicture(const Arguments& args){
  HandleScope scope;
  
  REQ_STR_ARG(0, path);
  REQ_FUN_ARG(1, cb);
  
  GPCamera *camera = ObjectWrap::Unwrap<GPCamera>(args.This());
  camera->Ref();
  take_picture_request *picture_req = new take_picture_request();
  picture_req->cb = Persistent<Function>::New(cb);
  
  picture_req->camera = camera->getCamera();
  picture_req->cameraObject = camera;
  picture_req->context = gp_context_new();
  picture_req->download = true;
  picture_req->path     = cv::CastFromJS<std::string>(args[0]);
  DO_ASYNC(picture_req, EIO_DownloadPicture, EIO_CapturePreviewCb);
  return Undefined();
}

void GPCamera::EIO_DownloadPicture(uv_work_t *_req){
  take_picture_request *req = (take_picture_request *)_req->data;
  
  req->cameraObject->lock();
  downloadPicture(req);
  req->cameraObject->unlock();  
}
// Return available configuration widgets as a list in the form
//  /main/status/model
//  /main/status/serialnumber
//  etc.
Handle<Value>
GPCamera::GetConfig(const Arguments& args) {
  HandleScope scope;
  REQ_FUN_ARG(0, cb)
  GPCamera *camera = ObjectWrap::Unwrap<GPCamera>(args.This());
  camera->Ref();
  get_config_request *config_req = new get_config_request();
  config_req->cameraObject = camera;
  config_req->camera = camera->getCamera();
  config_req->context = gp_context_new();
  config_req->cb = Persistent<Function>::New(cb);

  DO_ASYNC(config_req, EIO_GetConfig, EIO_GetConfigCb);
  
  //eio_custom(EIO_GetConfig, EIO_PRI_DEFAULT, EIO_GetConfigCb, config_req);
  //ev_ref(EV_DEFAULT_UC);
  return Undefined();  
}

namespace cvv8 {
  template<> struct NativeToJS<TreeNode>
    {
      v8::Handle<v8::Value> operator()( TreeNode const & node ) const
      {
        HandleScope scope;
        if(node.value){
          Handle<Value> value = GPCamera::getWidgetValue(node.context, node.value);
          if(value->IsObject()){
            Local<Object> obj = value->ToObject();
            if(node.subtree.size()){
              obj->Set(cvv8::CastToJS("children"), cvv8::CastToJS(node.subtree));
            }else{  
              obj->Set(cvv8::CastToJS("children"), Undefined());
            }
          }
          return scope.Close(value);
        }else{
          return scope.Close(Undefined());
        }
      }
    };
}



void GPCamera::EIO_GetConfig(uv_work_t *req){
  get_config_request *config_req = (get_config_request*)req->data;
  int ret;

  config_req->cameraObject->lock();
  ret = gp_camera_get_config(config_req->camera, &config_req->root, config_req->context);
  config_req->cameraObject->unlock();

  if(ret < GP_OK){
    config_req->ret = ret;
  }else{
    ret = enumConfig(config_req, config_req->root, config_req->settings);
    config_req->ret = ret;
  }
}
void GPCamera::EIO_GetConfigCb(uv_work_t *req){
  HandleScope scope;
  get_config_request *config_req = (get_config_request*)req->data;
  
  Handle<Value> argv[2];
  
  
  if(config_req->ret == GP_OK){
    argv[0] = Undefined();
    argv[1] = cv::CastToJS(config_req->settings);
  }
  else{
    argv[0] = cv::CastToJS(config_req->ret);
    argv[1] = Undefined();
  }

  config_req->cb->Call(Context::GetCurrent()->Global(), 2, argv);
  
  gp_widget_free(config_req->root);
  config_req->cb.Dispose();  
  config_req->cameraObject->Unref();
  gp_context_unref(config_req->context);
  
  delete config_req;
}

Handle<Value>
GPCamera::SetConfigValue(const Arguments& args) {
  HandleScope scope;
  GPCamera *camera = ObjectWrap::Unwrap<GPCamera>(args.This());
  camera->Ref();
  
  REQ_ARGS(3);
  REQ_STR_ARG(0, key);
  REQ_FUN_ARG(2, cb);

  set_config_request *config_req = new set_config_request();
  if(args[1]->IsString()){
    REQ_STR_ARG(1, value);
    config_req->strValue = *value;
    config_req->valueType = set_config_request::String;
  }else if(args[1]->IsInt32()){
    REQ_INT_ARG(1, value);
    config_req->intValue = value;
    config_req->valueType = set_config_request::Integer;
  }else if(args[1]->IsNumber()){
    double dblValue = args[1]->ToNumber()->Value();
    config_req->fltValue = dblValue;
    config_req->valueType = set_config_request::Float;
  }else{
    delete config_req;
    return ThrowException(Exception::TypeError(String::New("Argument 1 invalid: String, Integer or Float value expected")));    
  }
  config_req->cameraObject = camera;
  config_req->camera = camera->getCamera();
  config_req->context = gp_context_new();
  config_req->cb = Persistent<Function>::New(cb);
  config_req->key = *key;
  DO_ASYNC(config_req, EIO_SetConfigValue, EIO_SetConfigValueCb);
  
  return Undefined();
}
void
GPCamera::EIO_SetConfigValue(uv_work_t *req){
  set_config_request *config_req = (set_config_request *)req->data;
  
  config_req->cameraObject->lock();
  config_req->ret = setWidgetValue(config_req);
  config_req->cameraObject->unlock();
}
void
GPCamera::EIO_SetConfigValueCb(uv_work_t *req){
  HandleScope scope;
  set_config_request *config_req = (set_config_request *)req->data;
  int argc = 0;
  Local<Value> argv[1];
  if(config_req->ret < GP_OK){
    argv[0] = Integer::New(config_req->ret);
    argc = 1;  
  }
  config_req->cb->Call(Context::GetCurrent()->Global(), argc, argv);
  config_req->cb.Dispose();  
  config_req->cameraObject->Unref();
  gp_context_unref(config_req->context);
  delete config_req;
  
}

Handle<Value>
GPCamera::New(const Arguments& args) {
  HandleScope scope;
  
  REQ_EXT_ARG(0, js_gphoto);
  REQ_STR_ARG(1, model_);
  REQ_STR_ARG(2, port_);

  GPCamera *camera = new GPCamera(js_gphoto, (std::string)*model_, (std::string)*port_);
  camera->Wrap(args.This());
  Local<Object> This = args.This();
  This->Set(String::New("model"),String::New(camera->model_.c_str()));
  This->Set(String::New("port"),String::New(camera->port_.c_str()));
  return args.This();
}  

Camera* GPCamera::getCamera(){
    //printf("getCamera %s gphoto=%p\n", this->isOpen() ? "open" : "closed", gp);
  if(!this->isOpen()){
    //printf("Opening camera %s with portList=%p abilitiesList=%p\n", this->model_.c_str(),this->gphoto_->getPortInfoList(), this->gphoto_->getAbilitiesList());
    this->gphoto_->openCamera(this);
  } 
  return this->camera_;
};


Handle<Value>
GPCamera::GetPreview(const Arguments& args) {
  HandleScope scope;
  GPCamera *camera = ObjectWrap::Unwrap<GPCamera>(args.This());
  camera->Ref();
  REQ_FUN_ARG(0, cb);
  take_picture_request *preview_req = new take_picture_request();

  preview_req->cb = Persistent<Function>::New(cb);
  preview_req->camera = camera->getCamera();
  preview_req->context = gp_context_new();
  preview_req->cameraObject = camera;
  preview_req->download = true;
  DO_ASYNC(preview_req, EIO_CapturePreview, EIO_CapturePreviewCb);
//  eio_custom(EIO_CapturePreview, EIO_PRI_DEFAULT, EIO_CapturePreviewCb, preview_req);
//  ev_ref(EV_DEFAULT_UC);
  return scope.Close(Undefined());
    
}
void GPCamera::EIO_CapturePreview(uv_work_t *req){
  take_picture_request *preview_req = (take_picture_request*) req->data;
  
  RETURN_ON_ERROR(preview_req, gp_file_new, (&preview_req->file), {});

  preview_req->cameraObject->lock();
  RETURN_ON_ERROR(preview_req, gp_camera_capture_preview, (preview_req->camera, preview_req->file, preview_req->context), {gp_file_free(preview_req->file);preview_req->cameraObject->unlock();});
  preview_req->cameraObject->unlock();

  unsigned long int length;
  RETURN_ON_ERROR(preview_req, gp_file_get_data_and_size, (preview_req->file, &preview_req->data, &length), {gp_file_free(preview_req->file);});
  preview_req->length = (size_t)length;
}

void GPCamera::EIO_CapturePreviewCb(uv_work_t *req){
  HandleScope scope;
  take_picture_request *preview_req = (take_picture_request*) req->data;
  Handle<Value> argv[2];
  int argc = 1;
  argv[0] = Undefined();
  if(preview_req->ret < GP_OK){
    argv[0] = Integer::New(preview_req->ret);
  }
  else if(preview_req->data && preview_req->download) {
    argc = 2;
    node::Buffer* slowBuffer = node::Buffer::New(preview_req->length);
    if(preview_req->length){
      memmove(Buffer::Data(slowBuffer), preview_req->data, preview_req->length);
      Local<Object> globalObj = Context::GetCurrent()->Global();
      Local<Function> bufferConstructor = Local<Function>::Cast(globalObj->Get(v8::String::New("Buffer")));
      Handle<Value> constructorArgs[3] = { slowBuffer->handle_, v8::Integer::New(preview_req->length), v8::Integer::New(0) };
      Local<Object> actualBuffer = bufferConstructor->NewInstance(3, constructorArgs);
      argv[1] = actualBuffer;
    }
  }else{
    argc = 2;
    argv[1] = cv::CastToJS(preview_req->path);
  }
  
  preview_req->cb->Call(Context::GetCurrent()->Global(), argc, argv);
  preview_req->cb.Dispose();
  if(preview_req->ret == GP_OK)  gp_file_free(preview_req->file);
  preview_req->cameraObject->Unref();
  gp_context_unref(preview_req->context);
  
  delete preview_req;  
}

bool
GPCamera::close(){
  // this->gphoto_->Unref();
  if(this->camera_)
    return gp_camera_exit(this->camera_, this->gphoto_->getContext()) < GP_OK ? false : true;
  else
    return true;    
}
