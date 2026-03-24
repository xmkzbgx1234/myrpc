add_executable(consumer
  CallUserService.cc
  $<TARGET_OBJECTS:myrpc_example_proto>

)

target_include_directories(consumer PRIVATE "${MYRPC_USER_PROTO_OUT_DIR}")

target_link_libraries(consumer PRIVATE ${MYRPC_PROTOBUF_TARGET})

if(TARGET myrpc)
  target_link_libraries(consumer PRIVATE myrpc)
endif()
