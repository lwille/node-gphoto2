/* Copyright contributors of the node-gphoto2 project */

#include "./camera.h"
#include "./gphoto.h"

extern "C" {
    NAN_MODULE_INIT(init) {
        Nan::HandleScope scope;
        GPhoto2::Initialize(target);
        GPCamera::Initialize(target);
    }

    NODE_MODULE(gphoto2, init)
}

