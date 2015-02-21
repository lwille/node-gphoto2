/* Copyright contributors of the node-gphoto2 project */


#include "./binding.h"

#include "./camera.h"
#include "./gphoto.h"

extern "C" {
  void init(v8::Handle<v8::Object> target) {
    NanScope();
    GPhoto2::Initialize(target);
    GPCamera::Initialize(target);
  }

  NODE_MODULE(gphoto2, init);
}
