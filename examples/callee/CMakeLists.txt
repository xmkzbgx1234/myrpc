add_executable(provider
  UserService.cc
  ../user.pb.cc
)

target_link_libraries(provider PRIVATE ${MYRPC_PROTOBUF_TARGET})

if(TARGET myrpc)
  target_link_libraries(provider PRIVATE myrpc)
endif()
