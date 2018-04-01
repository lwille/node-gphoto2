cmake_minimum_required(VERSION 3.1)

project(node_gphoto2 CXX)

include(NodeJS.cmake)
include(FindGphoto2.cmake)
nodejs_init()

add_nodejs_module(${PROJECT_NAME} ${PROJECT_SOURCE_DIR})
set_target_properties(${PROJECT_NAME} PROPERTIES LINKER_LANGUAGE CXX)
set_target_properties(${PROJECT_NAME} PROPERTIES CMAKE_BUILD_TYPE Debug)
