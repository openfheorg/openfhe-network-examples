cmake_minimum_required(VERSION 3.13)
set(CMAKE_VERBOSE_MAKEFILE ON)

# Add the GRPC path to the prefix path
set(CMAKE_PREFIX_PATH "${HOME}/.local")
# Allows importing grpc
include(./common.cmake)

#configure_file(config.h.in config.h @ONLY) # to generate the config.h file in the build directory

# Include generated *.pb.h files and config.h
set(CMAKE_INCLUDE_CURRENT_DIR ON) # CMAKE_CURRENT_SOURCE_DIR and CMAKE_CURRENT_BINARY_DIR are added to the include path for each directory
include_directories(../../src/peer_to_peer_framework/include)
include_directories(${CMAKE_BINARY_DIR}/src/peer_to_peer_framework)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/bin)

message(STATUS "***************************************************************************")
message(STATUS "Binary dir: " ${CMAKE_BINARY_DIR})
message(STATUS "Source dir: " ${CMAKE_SOURCE_DIR})

# additional source files
#file(GLOB SRC_FILES ${CMAKE_CURRENT_SOURCE_DIR}/src/*.cpp)
# file(GLOB SRC_FILES RELATIVE ${CMAKE_CURRENT_SOURCE_DIR}/src/*.cpp)
message(STATUS "CMAKE_CURRENT_SOURCE_DIR: " ${CMAKE_CURRENT_SOURCE_DIR})

# Targets pre_net_(client|server)
file(GLOB TARGET_FILES CONFIGURE_DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/targets/*.cpp)

foreach (_file ${TARGET_FILES})
    message(STATUS "BUILDING FOR FILE: " ${_file})
    get_filename_component(_target ${_file} NAME_WE)
    add_executable(${_target} ${_file} ${SRC_FILES})
    target_sources(${_target} PRIVATE)
    target_link_libraries(${_target}
            p2p_grpc_proto
            ${_REFLECTION}
            ${_GRPC_GRPCPP}
            ${_PROTOBUF_LIBPROTOBUF})
endforeach ()
