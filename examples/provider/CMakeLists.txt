add_executable(provider
  main.cc
  $<TARGET_OBJECTS:myrpc_example_proto>
)

target_include_directories(provider PRIVATE "${MYRPC_USER_PROTO_OUT_DIR}")

target_link_libraries(provider PRIVATE ${MYRPC_PROTOBUF_TARGET} myrpc::myrpc)
