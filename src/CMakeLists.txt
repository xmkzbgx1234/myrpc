set(MYRPC_RPC_PROTO "${PROJECT_SOURCE_DIR}/proto/rpcHeader.proto")
set(MYRPC_RPC_PROTO_OUT_DIR "${CMAKE_CURRENT_BINARY_DIR}/generated/protocol")

myrpc_generate_proto(
  MYRPC_RPC_PROTO_SRC
  MYRPC_RPC_PROTO_HDR
  "${MYRPC_RPC_PROTO}"
  "${MYRPC_RPC_PROTO_OUT_DIR}"
  "${PROJECT_SOURCE_DIR}/proto"
  "Generating rpcHeader protobuf sources"
)

add_library(myrpc
  application/MyRpcApplication.cc
  provider/MyRpcProvider.cc
  consumer/MyRpcChannel.cc
  consumer/MyRpcController.cc
  registry/RedisRegistry.cc
  "${MYRPC_RPC_PROTO_SRC}"
)

add_library(myrpc::myrpc ALIAS myrpc)

target_include_directories(myrpc
  PUBLIC
    ${PROJECT_SOURCE_DIR}/include
    ${PROJECT_SOURCE_DIR}/common
    ${MYRPC_RPC_PROTO_OUT_DIR}
)

target_link_libraries(myrpc
  PUBLIC
    ${MYRPC_PROTOBUF_TARGET}
    myrpc::common
)

if(TARGET myrpc::muduo)
  target_link_libraries(myrpc PUBLIC myrpc::muduo)
endif()

if(TARGET myrpc::hiredis)
  target_link_libraries(myrpc PUBLIC myrpc::hiredis)
endif()

target_compile_features(myrpc PUBLIC cxx_std_17)
