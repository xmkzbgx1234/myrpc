cmake_minimum_required(VERSION 3.16)
project(myrpc LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

set(CMAKE_BUILD_TYPE Debug)

option(MYRPC_BUILD_TESTS "Build all tests" ON)
option(MYRPC_BUILD_PROTOBUF_TESTS "Build protobuf tests" ON)
option(MYRPC_BUILD_EXAMPLES "Build example applications" ON)
option(MYRPC_USE_MUDUO "Enable locally installed muduo support" ON)

set(EXECUTABLE_OUTPUT_PATH ${PROJECT_SOURCE_DIR}/bin)
set(LIBRARY_OUTPUT_PATH ${PROJECT_SOURCE_DIR}/lib)

find_package(Protobuf CONFIG QUIET)

if(TARGET protobuf::libprotobuf)
  set(MYRPC_PROTOBUF_TARGET protobuf::libprotobuf)
elseif(TARGET Protobuf::protobuf)
  set(MYRPC_PROTOBUF_TARGET Protobuf::protobuf)
else()
  find_package(PkgConfig REQUIRED)
  set(_local_pkgconfig "$ENV{HOME}/.local/lib/pkgconfig")
  if(EXISTS "${_local_pkgconfig}")
    if(DEFINED ENV{PKG_CONFIG_PATH} AND NOT "$ENV{PKG_CONFIG_PATH}" STREQUAL "")
      set(ENV{PKG_CONFIG_PATH} "${_local_pkgconfig}:$ENV{PKG_CONFIG_PATH}")
    else()
      set(ENV{PKG_CONFIG_PATH} "${_local_pkgconfig}")
    endif()
  endif()
  pkg_check_modules(PROTOBUF REQUIRED IMPORTED_TARGET protobuf)
  set(MYRPC_PROTOBUF_TARGET PkgConfig::PROTOBUF)
endif()

find_program(PROTOC_EXECUTABLE NAMES protoc REQUIRED)
find_path(HIREDIS_INCLUDE_DIR hiredis/hiredis.h)
find_library(HIREDIS_LIBRARY hiredis)

if(HIREDIS_INCLUDE_DIR AND HIREDIS_LIBRARY)
  add_library(myrpc_hiredis INTERFACE)
  add_library(myrpc::hiredis ALIAS myrpc_hiredis)
  target_include_directories(myrpc_hiredis INTERFACE ${HIREDIS_INCLUDE_DIR})
  target_link_libraries(myrpc_hiredis INTERFACE ${HIREDIS_LIBRARY})
endif()

function(myrpc_generate_proto out_src out_hdr proto_file out_dir proto_root comment_text)
  get_filename_component(_proto_name "${proto_file}" NAME_WE)
  set(_src "${out_dir}/${_proto_name}.pb.cc")
  set(_hdr "${out_dir}/${_proto_name}.pb.h")

  add_custom_command(
    OUTPUT "${_src}" "${_hdr}"
    COMMAND "${CMAKE_COMMAND}" -E make_directory "${out_dir}"
    COMMAND "${PROTOC_EXECUTABLE}"
            --cpp_out=${out_dir}
            --proto_path=${proto_root}
            "${proto_file}"
    DEPENDS "${proto_file}"
    COMMENT "${comment_text}"
    VERBATIM
  )

  set(${out_src} "${_src}" PARENT_SCOPE)
  set(${out_hdr} "${_hdr}" PARENT_SCOPE)
endfunction()

if(MYRPC_USE_MUDUO)
  set(MYRPC_MUDUO_ROOT "$ENV{HOME}/.local/muduo")
  set(MYRPC_LOCAL_BOOST_ROOT "$ENV{HOME}/.local/usr")
  set(MYRPC_LOCAL_BOOST_LIBDIR "${MYRPC_LOCAL_BOOST_ROOT}/lib/x86_64-linux-gnu")

  add_library(myrpc_muduo INTERFACE)
  add_library(myrpc::muduo ALIAS myrpc_muduo)

  target_include_directories(myrpc_muduo
    INTERFACE
      ${MYRPC_MUDUO_ROOT}/include
      ${MYRPC_LOCAL_BOOST_ROOT}/include
  )

  target_link_directories(myrpc_muduo
    INTERFACE
      ${MYRPC_MUDUO_ROOT}/lib
      ${MYRPC_LOCAL_BOOST_LIBDIR}
  )

  target_link_libraries(myrpc_muduo
    INTERFACE
      ${MYRPC_MUDUO_ROOT}/lib/libmuduo_net.a
      ${MYRPC_MUDUO_ROOT}/lib/libmuduo_base.a
      boost_thread
      boost_system
      pthread
      rt
  )

  list(APPEND CMAKE_BUILD_RPATH
    ${MYRPC_MUDUO_ROOT}/lib
    ${MYRPC_LOCAL_BOOST_LIBDIR}
  )
endif()

add_subdirectory(common)
if(EXISTS "${PROJECT_SOURCE_DIR}/src/CMakeLists.txt")
  add_subdirectory(src)
endif()
if(MYRPC_BUILD_EXAMPLES)
  add_subdirectory(examples)
endif()

if(MYRPC_BUILD_TESTS)
  include(CTest)
  add_subdirectory(tests)
endif()
