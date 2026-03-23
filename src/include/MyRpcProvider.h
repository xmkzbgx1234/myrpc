#pragma once

#include <google/protobuf/service.h>
#include <memory>
#include <muduo/net/TcpServer.h>
#include <muduo/net/EventLoop.h>
#include <muduo/net/InetAddress.h>
#include <unordered_map>
#include <functional>
#include <string>
#include <google/protobuf/descriptor.h>

class MyRpcProvider
{
private:
    muduo::net::EventLoop loop;
    // 处理Rpc请求的线程池对象
    void OnConnection(const muduo::net::TcpConnectionPtr& conn);
    void OnMessage(const muduo::net::TcpConnectionPtr& conn, muduo::net::Buffer* buffer,
        muduo::Timestamp receiveTime);

    struct ServiceInfo {
        google::protobuf::Service* service; // 保存服务对象
        std::unordered_map<std::string, const google::protobuf::MethodDescriptor*> methodMap; // 保存方法描述信息
    };
    std::unordered_map<std::string, ServiceInfo> serviceMap; // 保存服务对象和方法描述信息
public:
    void NotifyService(google::protobuf::Service* service);
    void Run();
    void sendRpcResponse(muduo::net::TcpConnectionPtr conn, google::protobuf::Message* response);
};
