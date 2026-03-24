set(MYRPC_RPC_PROTO "${PROJECT_SOURCE_DIR}/proto/rpcHeader.proto")
set(MYRPC_RPC_PROTO_OUT_DIR "${CMAKE_CURRENT_BINARY_DIR}/generated/protocol")
set(MYRPC_RPC_PROTO_SRC "${MYRPC_RPC_PROTO_OUT_DIR}/rpcHeader.pb.cc")
set(MYRPC_RPC_PROTO_HDR "${MYRPC_RPC_PROTO_OUT_DIR}/rpcHeader.pb.h")

add_custom_command(
  OUTPUT "${MYRPC_RPC_PROTO_SRC}" "${MYRPC_RPC_PROTO_HDR}"
  COMMAND "${CMAKE_COMMAND}" -E make_directory "${MYRPC_RPC_PROTO_OUT_DIR}"
  COMMAND "${PROTOC_EXECUTABLE}"
          --cpp_out=${MYRPC_RPC_PROTO_OUT_DIR}
          --proto_path=${PROJECT_SOURCE_DIR}/proto
          "${MYRPC_RPC_PROTO}"
  DEPENDS "${MYRPC_RPC_PROTO}"
  COMMENT "Generating rpcHeader protobuf sources"
  VERBATIM
)

add_library(myrpc
  application/MyRpcApplication.cc
  provider/MyRpcProvider.cc
  consumer/MyRpcChannel.cc
  consumer/MyRpcController.cc
  "${MYRPC_RPC_PROTO_SRC}"
)

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

target_compile_features(myrpc PUBLIC cxx_std_17)
