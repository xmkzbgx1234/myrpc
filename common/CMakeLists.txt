add_library(myrpc_common INTERFACE)
add_library(myrpc::common ALIAS myrpc_common)

target_include_directories(myrpc_common
  INTERFACE
    ${CMAKE_CURRENT_SOURCE_DIR}
)

target_compile_features(myrpc_common INTERFACE cxx_std_17)
