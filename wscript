import Options, Utils
from os import unlink, symlink, chdir
from os.path import exists, lexists

srcdir = "."
blddir = "build"
VERSION = "0.0.1"

PKG_CONFIG_PATH = "/usr/local/Cellar/libgphoto2/2.4.11/lib/pkgconfig/libgphoto2"
def set_options(opt):
  opt.tool_options("compiler_cxx")

def configure(conf):
  conf.check_tool("compiler_cxx")
  conf.check_tool("node_addon")  
  conf.check_cfg(package='libgphoto2', args='--cflags --libs', uselib_store='LIBGPHOTO2')
  conf.check_cfg(package='libgphoto2_port', args='--cflags --libs', uselib_store='LIBGPHOTO2PORT')
def build(bld):

  obj = bld.new_task_gen("cxx", "shlib", "node_addon")
  # without this, eio_custom doesn't keep a ref to the req->data
  obj.cxxflags = ["-D_FILE_OFFSET_BITS=64", "-D_LARGEFILE_SOURCE"]
  obj.target = "gphoto2"
  obj.source = [
                "src/binding.cc",
                "src/camera.cc",
                "src/gphoto.cc",
                "src/autodetect.cc"
               ]
  obj.uselib = "LIBGPHOTO2 LIBGPHOTO2PORT"

# def shutdown(bld):
#   if Options.commands['clean'] and not Options.commands['build']:
#     if lexists('gphoto2.node'):
#       unlink('gphoto2.node')
#   elif Options.commands['build']:
#     if exists('build/default/gphoto2.node') and not lexists('gphoto2.node'):
#       symlink('build/default/gphoto2.node', 'gphoto2.node')
#     if exists('build/Release/gphoto2.node') and not lexists('gphoto2.node'):
#       symlink('build/Release/gphoto2.node', 'gphoto2.node')