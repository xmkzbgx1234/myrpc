set(MYRPC_USER_PROTO "${PROJECT_SOURCE_DIR}/proto/user.proto")
set(MYRPC_USER_PROTO_OUT_DIR "${CMAKE_CURRENT_BINARY_DIR}/generated")

myrpc_generate_proto(
  MYRPC_USER_PROTO_SRC
  MYRPC_USER_PROTO_HDR
  "${MYRPC_USER_PROTO}"
  "${MYRPC_USER_PROTO_OUT_DIR}"
  "${PROJECT_SOURCE_DIR}/proto"
  "Generating user protobuf sources"
)

add_library(myrpc_example_proto OBJECT "${MYRPC_USER_PROTO_SRC}")
add_library(myrpc::example_proto ALIAS myrpc_example_proto)
target_include_directories(myrpc_example_proto PUBLIC "${MYRPC_USER_PROTO_OUT_DIR}")
target_link_libraries(myrpc_example_proto PUBLIC ${MYRPC_PROTOBUF_TARGET})

add_subdirectory(provider)
add_subdirectory(consumer)
