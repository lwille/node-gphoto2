/* Copyright 2012 Leonhardt Wille */

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
