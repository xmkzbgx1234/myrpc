add_library(myrpc
  include/MyRpcApplication.cc
  include/MyRpcProvider.cc
  include/MyRpcChannel.cc
  include/rpcHeader.pb.cc
  include/MyRpcController.cc
)

target_include_directories(myrpc
  PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/include
    ${PROJECT_SOURCE_DIR}/common
)

target_link_libraries(myrpc
  PUBLIC
    ${MYRPC_PROTOBUF_TARGET}
    myrpc::common
)

if(TARGET myrpc::muduo)
  target_link_libraries(myrpc PUBLIC myrpc::muduo)
endif()

target_compile_features(myrpc PUBLIC cxx_std_17)
