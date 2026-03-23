if(CMAKE_SOURCE_DIR STREQUAL CMAKE_CURRENT_SOURCE_DIR)
  cmake_minimum_required(VERSION 3.16)
  project(protobuf_test LANGUAGES CXX)
endif()

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_package(PkgConfig REQUIRED)

# Make local protobuf/absl .pc files discoverable in this project.
set(_local_pkgconfig "$ENV{HOME}/.local/lib/pkgconfig")
if(EXISTS "${_local_pkgconfig}")
  if(DEFINED ENV{PKG_CONFIG_PATH} AND NOT "$ENV{PKG_CONFIG_PATH}" STREQUAL "")
    set(ENV{PKG_CONFIG_PATH} "${_local_pkgconfig}:$ENV{PKG_CONFIG_PATH}")
  else()
    set(ENV{PKG_CONFIG_PATH} "${_local_pkgconfig}")
  endif()
endif()

pkg_check_modules(PROTOBUF REQUIRED IMPORTED_TARGET protobuf)
find_program(PROTOC_EXECUTABLE NAMES protoc REQUIRED)

set(PROTO_FILE "${CMAKE_CURRENT_SOURCE_DIR}/test.proto")
set(GENERATED_SRC "${CMAKE_CURRENT_BINARY_DIR}/test.pb.cc")
set(GENERATED_HDR "${CMAKE_CURRENT_BINARY_DIR}/test.pb.h")

add_custom_command(
  OUTPUT "${GENERATED_SRC}" "${GENERATED_HDR}"
  COMMAND "${PROTOC_EXECUTABLE}"
          --cpp_out=${CMAKE_CURRENT_BINARY_DIR}
          --proto_path=${CMAKE_CURRENT_SOURCE_DIR}
          "${PROTO_FILE}"
  DEPENDS "${PROTO_FILE}"
  COMMENT "Generating C++ source from test.proto"
  VERBATIM
)

add_executable(protobuf_test
  "${CMAKE_CURRENT_SOURCE_DIR}/main.cc"
  "${GENERATED_SRC}"
)

target_include_directories(protobuf_test PRIVATE "${CMAKE_CURRENT_BINARY_DIR}")
target_link_libraries(protobuf_test PRIVATE PkgConfig::PROTOBUF)

if(BUILD_TESTING)
  add_test(NAME protobuf_smoke COMMAND protobuf_test)
endif()
