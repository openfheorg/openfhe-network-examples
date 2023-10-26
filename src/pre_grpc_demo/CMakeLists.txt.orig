
cmake_minimum_required(VERSION 3.13)
set(CMAKE_VERBOSE_MAKEFILE ON)

### project(PreNet C CXX)

# set CMAKE_PREFIX_PATH to the location of the installed gRPC libs
# this variable should not be set if you link all libs pre_net depends on
# along with "pre_net"
set(CMAKE_PREFIX_PATH "${HOME}/.local")

include(./common.cmake)

# Proto file
get_filename_component(hw_proto "./protos/pre_net.proto" ABSOLUTE)
get_filename_component(hw_proto_path "${hw_proto}" PATH)

# Generated sources
set(hw_proto_srcs "${CMAKE_CURRENT_BINARY_DIR}/pre_net.pb.cc")
set(hw_proto_hdrs "${CMAKE_CURRENT_BINARY_DIR}/pre_net.pb.h")
set(hw_grpc_srcs "${CMAKE_CURRENT_BINARY_DIR}/pre_net.grpc.pb.cc")
set(hw_grpc_hdrs "${CMAKE_CURRENT_BINARY_DIR}/pre_net.grpc.pb.h")
add_custom_command(
      OUTPUT "${hw_proto_srcs}" "${hw_proto_hdrs}" "${hw_grpc_srcs}" "${hw_grpc_hdrs}"
      COMMAND ${_PROTOBUF_PROTOC}
      ARGS --grpc_out "${CMAKE_CURRENT_BINARY_DIR}"
        --cpp_out "${CMAKE_CURRENT_BINARY_DIR}"
        -I "${hw_proto_path}"
        --plugin=protoc-gen-grpc="${_GRPC_CPP_PLUGIN_EXECUTABLE}"
        "${hw_proto}"
      DEPENDS "${hw_proto}")

set(CERTS_AND_KEYS_DIR "${CMAKE_CURRENT_SOURCE_DIR}/authentication")
configure_file (config.h.in config.h @ONLY) # to generate the config.h file in the build directory

# Include generated *.pb.h files and config.h
set(CMAKE_INCLUDE_CURRENT_DIR ON) # CMAKE_CURRENT_SOURCE_DIR and CMAKE_CURRENT_BINARY_DIR are added to the include path for each directory
include_directories(${CMAKE_CURRENT_SOURCE_DIR}/include)

# additional source files
# set(SRC_FILES "${CMAKE_CURRENT_SOURCE_DIR}/src/*.cpp")
# file(GLOB SRC_FILES RELATIVE "${CMAKE_CURRENT_SOURCE_DIR}/src" "*.cpp")
#file(GLOB SRC_FILES CONFIGURE_DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/src/*.cpp)
file(GLOB SRC_FILES ${CMAKE_CURRENT_SOURCE_DIR}/src/*.cpp)
# file(GLOB SRC_FILES RELATIVE ${CMAKE_CURRENT_SOURCE_DIR}/src/*.cpp)
message (STATUS "===== CMAKE_CURRENT_SOURCE_DIR: " ${CMAKE_CURRENT_SOURCE_DIR})
message (STATUS "===== SRC_FILES: " ${SRC_FILES})

# hw_grpc_proto
add_library(hw_grpc_proto
  ${hw_grpc_srcs}
  ${hw_grpc_hdrs}
  ${hw_proto_srcs}
  ${hw_proto_hdrs}
  ${SRC_FILES})
target_link_libraries(hw_grpc_proto
  ${_REFLECTION}
  ${_GRPC_GRPCPP}
  ${_PROTOBUF_LIBPROTOBUF})

# Targets pre_net_(client|server)
file (GLOB TARGET_FILES CONFIGURE_DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/targets/*.cpp)
foreach(_file ${TARGET_FILES})
  get_filename_component (_target ${_file} NAME_WE )
  add_executable(${_target} ${_file})
  #add_executable(${_target} ${_file} ${SRC_FILES})
  target_sources(${_target} PRIVATE)
  target_link_libraries(${_target}
    hw_grpc_proto
    ${_REFLECTION}
    ${_GRPC_GRPCPP}
    ${_PROTOBUF_LIBPROTOBUF})
endforeach()

### get_property(dirs DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR} PROPERTY INCLUDE_DIRECTORIES)
### message (STATUS "===== INCLUDE_DIRECTORIES directories:")
### foreach(dir ${dirs})
###   message(STATUS "dir='${dir}'")
### endforeach()
