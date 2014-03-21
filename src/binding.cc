/* Copyright contributors of the node-gphoto2 project */

#include "camera.h"  // NOLINT
#include "gphoto.h"  // NOLINT

extern "C" {
  void init(Handle<Object> target) {
    HandleScope scope;
    GPhoto2::Initialize(target);
    GPCamera::Initialize(target);
  }

  NODE_MODULE(gphoto2, init);
}
