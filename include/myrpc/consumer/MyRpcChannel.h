#pragma once
#include <google/protobuf/service.h>
#include <google/protobuf/message.h>
#include <google/protobuf/descriptor.h>
#include <sys/socket.h>
#include <sys/select.h>
#include <fcntl.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <vector>
#include <atomic>
#include <string>
#include <errno.h>
#include <string.h>
#include <sys/types.h>

#include "myrpc/protocol/ErrorCode.h"
#include "rpcHeader.pb.h"
#include "log.h"
#include "myrpc/registry/RedisRegistry.h"

class MyRpcChannel : public google::protobuf::RpcChannel {
public:
    // 所有方法都通过这个函数调用 MyRpcProvider::CallMethod
    void CallMethod(const google::protobuf::MethodDescriptor* method,
                    google::protobuf::RpcController* controller,
                    const google::protobuf::Message* request,
                    google::protobuf::Message* response,
                    google::protobuf::Closure* done);
};
