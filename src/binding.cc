#include "gphoto.h"
#include "camera.h"

extern "C" void
init(Handle<Object> target)
{
    HandleScope scope;
    GPhoto2::Initialize(target);
    GPCamera::Initialize(target);
}

NODE_MODULE(gphoto2, init);


