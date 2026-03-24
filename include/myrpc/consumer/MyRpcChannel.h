#pragma once
#include <google/protobuf/service.h>
#include <google/protobuf/message.h>
#include <google/protobuf/descriptor.h>

class MyRpcChannel : public google::protobuf::RpcChannel {
public:
    // 所有方法都通过这个函数调用 MyRpcProvider::CallMethod
    void CallMethod(const google::protobuf::MethodDescriptor* method,
                    google::protobuf::RpcController* controller,
                    const google::protobuf::Message* request,
                    google::protobuf::Message* response,
                    google::protobuf::Closure* done);
};
