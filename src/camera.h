#ifndef __CAMERA_H
#define __CAMERA_H
  #include "binding.h"
  #include "gphoto.h"
  namespace cv = cvv8;

  // Class for templated typedef's
  // Initialize as A<Typename>::Tree etc.
  template <typename T>
  class A {
  private:
    A(void) {};
  public:
    typedef std::map<std::string, T> Tree;
  };


  struct TreeNode{
    CameraWidget* value;
    GPContext *context;
    A<TreeNode>::Tree subtree;
    TreeNode(){};
  };

  using namespace v8;
  typedef std::list<std::string> StringList;

  static Persistent<String> camera_getConfig_symbol;
  static Persistent<String> camera_getConfigValue_symbol;
  static Persistent<String> camera_setConfigValue_symbol;
  static Persistent<String> camera_takePicture_symbol;
  static Persistent<String> camera_downloadPicture_symbol;
  class GPCamera : public node::ObjectWrap {
    pthread_mutex_t cameraMutex;
    void lock(){pthread_mutex_lock(&this->cameraMutex);};
    void unlock(){pthread_mutex_unlock(&this->cameraMutex);};

    std::string model_;
    std::string port_;
    Persistent<External> gphoto;
    GPhoto2    *gphoto_;
    Camera     *camera_;
  	CameraWidget *config_;
    bool isOpen(){return this->camera_ ? true : false;};

    struct take_picture_request {
      Persistent<Function> cb;
      Camera *camera;
      GPCamera *cameraObject;
      CameraFile *file;
      GPContext *context;
      const char *data;
      unsigned long int length;
      int ret;
      bool download;
      bool preview;
      std::string path;
      std::string target_path;
      std::string socket_path;
    };
    struct get_config_request {
      Persistent<Function> cb;
      GPCamera  *cameraObject;
      Camera    *camera;
      GPContext *context;
      CameraWidget *root;
      int ret;
      StringList keys;
      A<TreeNode>::Tree settings;
    };
    struct set_config_request {
      Persistent<Function> cb;
      GPCamera  *cameraObject;
      Camera    *camera;
      GPContext *context;
      std::string key;
      enum {String,Float,Integer} valueType;
      float fltValue;
      int intValue;
      std::string strValue;
      int ret;
    };
    static int enumConfig(get_config_request* req, CameraWidget *root, A<TreeNode>::Tree &tree);
    static int getConfigWidget(get_config_request *req, std::string name, CameraWidget **child, CameraWidget **rootconfig);
    static int setWidgetValue(set_config_request *req);
    static void takePicture(take_picture_request *req);
    static void capturePreview(take_picture_request *req);
    static void downloadPicture(take_picture_request *req);
    static int getCameraFile(take_picture_request *req, CameraFile **file);
    bool close();

    public:
      GPCamera(Handle<External> js_gphoto, std::string  model, std::string  port);
      ~GPCamera();
      static Handle<Value> getWidgetValue(GPContext *context, CameraWidget *widget);
      static Persistent<FunctionTemplate> constructor_template;
      static void Initialize(Handle<Object> target);
      static Handle<Value> New(const Arguments& args);
      static Handle<Value> GetConfig(const Arguments& args);
      static Handle<Value> GetConfigValue(const Arguments &args);
      static Handle<Value> SetConfigValue(const Arguments &args);
      static Handle<Value> TakePicture(const Arguments &args);
      static Handle<Value> DownloadPicture(const Arguments& args);
      ASYNC_FN(EIO_GetConfig);
      ASYNC_CB(EIO_GetConfigCb);
      ASYNC_FN(EIO_SetConfigValue);
      ASYNC_CB(EIO_SetConfigValueCb);
      ASYNC_FN(EIO_DownloadPicture);
      ASYNC_FN(EIO_Capture);
      ASYNC_CB(EIO_CaptureCb);
      std::string getPort(){return this->port_;};
      std::string getModel(){return this->model_;};
      void setCamera(Camera *camera){this->camera_=camera;};
      Camera* getCamera();
  };

#endif