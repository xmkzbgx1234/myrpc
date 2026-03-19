add_executable(callee
  UserService.cc
  ../user.pb.cc
)

target_link_libraries(callee PRIVATE
  protobuf::libprotobuf
)
