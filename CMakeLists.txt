cmake_minimum_required(VERSION 3.16)
project(myrpc LANGUAGES CXX) 

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

option(MYRPC_BUILD_TESTS "Build all tests" ON)
option(MYRPC_BUILD_PROTOBUF_TESTS "Build protobuf tests" ON)

set(EXECUTABLE_OUTPUT_PATH ${PROJECT_SOURCE_DIR}/bin)
set(LIBRARY_OUTPUT_PATH ${PROJECT_SOURCE_DIR}/lib)

find_package(Protobuf REQUIRED)
add_subdirectory(examples)
add_subdirectory(src)

include_directories(examples)

# add_subdirectory(common)

if(MYRPC_BUILD_TESTS)
  include(CTest)
  add_subdirectory(tests)
endif()
