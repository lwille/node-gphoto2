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
}
GPCamera::~GPCamera(){
  printf("Camera destructor\n");
  this->gphoto_->closeCamera(this);
  this->gphoto.Dispose();
  this->close();
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
    ADD_PROTOTYPE_METHOD(camera, getConfigValue, GetConfigValue);
    ADD_PROTOTYPE_METHOD(camera, setConfigValue, SetConfigValue);
    ADD_PROTOTYPE_METHOD(camera, takePicture, TakePicture);
    ADD_PROTOTYPE_METHOD(camera, getPreview, GetPreview);
    target->Set(String::NewSymbol("Camera"), constructor_template->GetFunction());
}

Handle<Value>
GPCamera::TakePicture(const Arguments& args) {
  HandleScope scope;
  printf("TakePicture %d\n", __LINE__);
  GPCamera *camera = ObjectWrap::Unwrap<GPCamera>(args.This());
  REQ_FUN_ARG(0, cb);
  take_picture_request *picture_req = new take_picture_request();
  picture_req->cb = Persistent<Function>::New(cb);
  picture_req->camera = camera->getCamera();
  picture_req->context = gp_context_new();
  DO_ASYNC(picture_req, EIO_TakePicture, EIO_TakePictureCb);
  // eio_custom(EIO_TakePicture, EIO_PRI_DEFAULT, EIO_TakePictureCb, picture_req);
  // ev_ref(EV_DEFAULT_UC);
  return Undefined();
}
void
GPCamera::EIO_TakePicture(uv_work_t *req){
  take_picture_request *picture_req = (take_picture_request *)req->data;
  printf("Taking picture\n");
  capture_to_memory(picture_req->camera, picture_req->context, &picture_req->data, static_cast<long unsigned int *>(&picture_req->length));
}
void
GPCamera::EIO_TakePictureCb(uv_work_t *req){
  HandleScope scope;
  take_picture_request *picture_req = (take_picture_request *)req->data;
  
  node::Buffer* buffer = node::Buffer::New((char*)picture_req->data, picture_req->length);
  
  Handle<Value> argv[1];
  argv[0] = buffer->handle_;
  
  picture_req->cb->Call(Context::GetCurrent()->Global(), 1, argv);
  picture_req->cb.Dispose();
  gp_context_unref(picture_req->context);
  delete picture_req;
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
  
  
  ret = gp_camera_get_config(config_req->camera, &config_req->root, config_req->context);
   if(ret == GP_ERROR_CAMERA_BUSY){
      usleep(25 * 1000);
      printf("retrying EIO_GetConfig after 25ms\n");
      EIO_GetConfig(req);
    }else if(ret < GP_OK){
      printf("gp_camera_get_config returned %d\n", ret);
      usleep(25 * 1000);
      EIO_GetConfig(req);      
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
GPCamera::GetConfigValue(const Arguments& args) {
  HandleScope scope;
  GPCamera *camera = ObjectWrap::Unwrap<GPCamera>(args.This());
  camera->Ref();

  REQ_ARR_ARG(0, js_keys);
  REQ_FUN_ARG(1, cb);

  get_config_request *config_req = new get_config_request();
  config_req->cameraObject = camera;
  config_req->camera       = camera->getCamera();
  config_req->cb = Persistent<Function>::New(cb);
  
  // transform keys Array into a stl::deque

  std::list<std::string> keys(cv::CastFromJS<std::list<std::string> >(js_keys));
  config_req->keys = keys;
  
  DO_ASYNC(config_req, EIO_GetConfigValue, EIO_GetConfigValueCb);
  
  
//  eio_custom(EIO_GetConfigValue, EIO_PRI_DEFAULT, EIO_GetConfigValueCb, config_req);
//  ev_ref(EV_DEFAULT_UC);
  
  //printf("Retrieving %d settings\n", keys->Length());
  
  return Undefined();
}



void
GPCamera::EIO_GetConfigValue(uv_work_t *req){
  printf("EIO_GetConfigValue\n");
  int ret;
  get_config_request *config_req = (get_config_request *)req->data;
  CameraWidget *root;
  for(StringList::iterator i=config_req->keys.begin(); i!=config_req->keys.end(); ++i){
    CameraWidget *widget = NULL;
    ret = getConfigWidget(config_req, *i, &widget, &root);
    if(ret == GP_ERROR_CAMERA_BUSY){
      usleep(25 * 1000);
      printf("retrying getConfigWidget after 25ms\n");
      EIO_GetConfigValue(req);
    }else if(ret < GP_OK){
      printf("getConfigWidget returned %d\n", ret);
      usleep(25 * 1000);
      EIO_GetConfigValue(req);      
    }else{
      TreeNode node;
      node.context = config_req->context;
      if(ret == GP_OK){
        node.value = widget;
      }else{
        node.value = NULL;
      }
      config_req->settings[*i] = node;    
    }
  }
}
void
GPCamera::EIO_GetConfigValueCb(uv_work_t *req){
  HandleScope scope;
  get_config_request *config_req = (get_config_request *)req->data;
  
  Handle<Value> argv[2];
  
  
  argv[0] = Undefined();
  Local<Object> values = Object::New();
  for(A<TreeNode>::Tree::iterator i  = config_req->settings.begin(); i != config_req->settings.end(); ++i){
    printf("Getting value for %s\n", (*i).first.c_str());
    TreeNode node =  (*i).second;
    values->Set(cv::CastToJS((*i).first), getWidgetValue(config_req->context, node.value));
  }
  argv[1] = values;
  
  config_req->cb->Call(Context::GetCurrent()->Global(), 2, argv);
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
  set_config_request *config_req = new set_config_request();
  
  config_req->camera = camera;
  REQ_STR_ARG(0, key);
  REQ_STR_ARG(0, value);
  REQ_FUN_ARG(1, cb);
  DO_ASYNC(config_req, EIO_SetConfigValue, EIO_SetConfigValueCb);
  
  return Undefined();
}
void
GPCamera::EIO_SetConfigValue(uv_work_t *req){
  get_config_request *config_req = (get_config_request *)req->data;
  
}
void
GPCamera::EIO_SetConfigValueCb(uv_work_t *req){
  HandleScope scope;
  get_config_request *config_req = (get_config_request *)req->data;
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
  GPhoto2 *gp = this->gphoto_;
    //printf("getCamera %s gphoto=%p\n", this->isOpen() ? "open" : "closed", gp);
  if(!this->isOpen()){
    printf("Opening camera %s with portList=%p abilitiesList=%p\n", this->model_.c_str(),this->gphoto_->getPortInfoList(), this->gphoto_->getAbilitiesList());
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
  
  DO_ASYNC(preview_req, EIO_CapturePreview, EIO_CapturePreviewCb);
//  eio_custom(EIO_CapturePreview, EIO_PRI_DEFAULT, EIO_CapturePreviewCb, preview_req);
//  ev_ref(EV_DEFAULT_UC);
  return scope.Close(Undefined());
    
}
void GPCamera::EIO_CapturePreview(uv_work_t *req){
  int ret;

  take_picture_request *preview_req = (take_picture_request*) req->data;
  
  RETURN_ON_ERROR(preview_req, gp_file_new, (&preview_req->file), {});
  RETURN_ON_ERROR(preview_req, gp_camera_capture_preview, (preview_req->camera, preview_req->file, preview_req->context), {
    if(preview_req->ret == GP_ERROR_CAMERA_BUSY) {
      usleep(25 * 1000);
      printf("Retrying gp_capture_preview after 25ms\n");
      EIO_CapturePreview(req);
    }else{
      gp_file_free(preview_req->file);
    }
  });
  unsigned long int length;
  RETURN_ON_ERROR(preview_req, gp_file_get_data_and_size, (preview_req->file, &preview_req->data, &length), {gp_file_free(preview_req->file);});
  preview_req->length = (size_t)length;
  
}

void GPCamera::EIO_CapturePreviewCb(uv_work_t *req){
  HandleScope scope;
  int ret;
  CameraFile *file;
  take_picture_request *preview_req = (take_picture_request*) req->data;
  Handle<Value> argv[2];
  int argc = 1;
  if(preview_req->ret < GP_OK){
    argv[0] = Integer::New(preview_req->ret);
  }
  else if(preview_req->data) {
    argc = 2;
    argv[0] = Undefined();
    node::Buffer* slowBuffer = node::Buffer::New(preview_req->length);
    memcpy(Buffer::Data(slowBuffer), preview_req->data, preview_req->length);
    Local<Object> globalObj = Context::GetCurrent()->Global();
    Local<Function> bufferConstructor = Local<Function>::Cast(globalObj->Get(v8::String::New("Buffer")));
    Handle<Value> constructorArgs[3] = { slowBuffer->handle_, v8::Integer::New(preview_req->length), v8::Integer::New(0) };
    Local<Object> actualBuffer = bufferConstructor->NewInstance(3, constructorArgs);
    argv[1] =actualBuffer;
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
