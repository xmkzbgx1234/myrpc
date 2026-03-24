set(MYRPC_USER_PROTO "${PROJECT_SOURCE_DIR}/proto/user.proto")
set(MYRPC_USER_PROTO_OUT_DIR "${CMAKE_CURRENT_BINARY_DIR}/generated")
set(MYRPC_USER_PROTO_SRC "${MYRPC_USER_PROTO_OUT_DIR}/user.pb.cc")
set(MYRPC_USER_PROTO_HDR "${MYRPC_USER_PROTO_OUT_DIR}/user.pb.h")

add_custom_command(
  OUTPUT "${MYRPC_USER_PROTO_SRC}" "${MYRPC_USER_PROTO_HDR}"
  COMMAND "${CMAKE_COMMAND}" -E make_directory "${MYRPC_USER_PROTO_OUT_DIR}"
  COMMAND "${PROTOC_EXECUTABLE}"
          --cpp_out=${MYRPC_USER_PROTO_OUT_DIR}
          --proto_path=${PROJECT_SOURCE_DIR}/proto
          "${MYRPC_USER_PROTO}"
  DEPENDS "${MYRPC_USER_PROTO}"
  COMMENT "Generating user protobuf sources"
  VERBATIM
)

add_library(myrpc_example_proto OBJECT "${MYRPC_USER_PROTO_SRC}")
target_include_directories(myrpc_example_proto PUBLIC "${MYRPC_USER_PROTO_OUT_DIR}")
target_link_libraries(myrpc_example_proto PUBLIC ${MYRPC_PROTOBUF_TARGET})

add_subdirectory(provider)
add_subdirectory(consumer)
