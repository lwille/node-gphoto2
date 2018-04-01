include(LibFindMacros.cmake)
find_path(GPHOTO2_INCLUDE_DIR gphoto2/gphoto2.h)
mark_as_advanced(GPHOTO2_INCLUDE_DIR)

set(GPHOTO2_NAMES ${GPHOTO2_NAMES} gphoto2 libgphoto2)
set(GPHOTO2_PORT_NAMES ${GPHOTO2_PORT_NAMES} gphoto2_port libgphoto2_port)
find_library(GPHOTO2_LIBRARY NAMES ${GPHOTO2_NAMES})
find_library(GPHOTO2_PORT_LIBRARY NAMES ${GPHOTO2_PORT_NAMES})
mark_as_advanced(GPHOTO2_LIBRARY)
mark_as_advanced(GPHOTO2_PORT_LIBRARY)

# Detect libgphoto2 version
libfind_pkg_check_modules(PC_GPHOTO2 libgphoto2)
if(PC_GPHOTO2_FOUND)
    set(GPHOTO2_VERSION_STRING "${PC_GPHOTO2_VERSION}")
endif()