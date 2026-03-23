add_executable(consumer
  CallUserService.cc
  ../user.pb.cc

)


target_link_libraries(consumer PRIVATE ${MYRPC_PROTOBUF_TARGET})

if(TARGET myrpc)
  target_link_libraries(consumer PRIVATE myrpc)
endif()
