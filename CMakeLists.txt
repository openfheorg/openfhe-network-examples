cmake_minimum_required (VERSION 3.5.1)

### To use gcc/g++ on a Macintosh, you must set the Compilers
### here, not inside the project
##if(APPLE)
##       set(CMAKE_C_COMPILER "/usr/local/bin/gcc-7")
##       set(CMAKE_CXX_COMPILER "/usr/local/bin/g++-7")
##endif()
### TODO: for now, we use CLang for Mac

project(OPENFHE-NETWORK-EXAMPLES CXX)

set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)

set(CMAKE_CXX_STANDARD 17) 
set(CMAKE_CXX_STANDARD_REQUIRED ON)
option( BUILD_STATIC "Set to ON to include static versions of the library" OFF)

find_package(OpenFHE)
message( STATUS "OpenFHE_LIBDIR=${OpenFHE_LIBDIR}" )


set(CMAKE_CXX_FLAGS ${OpenFHE_CXX_FLAGS})

include_directories( ${OPENMP_INCLUDES} )
include_directories( ${OpenFHE_INCLUDE} )
include_directories( ${OpenFHE_INCLUDE}/third-party/include )
include_directories( ${OpenFHE_INCLUDE}/core )
include_directories( ${OpenFHE_INCLUDE}/pke )
include_directories( ${OpenFHE_INCLUDE}/binfhe )
### add directories for other OpenFHE modules as needed for your project

link_directories( ${OpenFHE_LIBDIR} )
link_directories( ${OPENMP_LIBRARIES} )
if(BUILD_STATIC)
  set( CMAKE_EXE_LINKER_FLAGS "${OpenFHE_EXE_LINKER_FLAGS} -static")
  link_libraries( ${OpenFHE_STATIC_LIBRARIES} )
else()
  set( CMAKE_EXE_LINKER_FLAGS ${OpenFHE_EXE_LINKER_FLAGS} )
  link_libraries( ${OpenFHE_SHARED_LIBRARIES} )
endif()
set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} -O0 -g")

### ADD YOUR EXECUTABLE(s) HERE
add_subDirectory(src/pre_grpc_demo)
add_subDirectory(src/peer_to_peer_framework)
add_subDirectory(src/threshold_fhe_aborts)
add_subDirectory(src/threshnet_aborts_grpc)
add_subDirectory(src/threshnet_aborts_p2p_demo)
add_subDirectory(src/thresh_net_adjacent_measure)
add_subDirectory(src/network_measure_examples)
option(BUILD_UNITTESTS "Set OFF to prevent building the unit tests" OFF)
message(STATUS "BUILD_TESTS:  ${BUILD_UNITTESTS}")
### Our tests
if (BUILD_UNITTESTS)
add_subdirectory(tests/two_party)
add_subdirectory(tests/n_party)
endif()
