#include "myrpc/provider/MyRpcProvider.h"
#include "myrpc/application/MyRpcApplication.h"
#include "log.h"
#include "rpcHeader.pb.h"

void MyRpcProvider::NotifyService(google::protobuf::Service* service) {
    // 把服务对象发布到rpc节点上
    ServiceInfo serviceInfo;
    const google::protobuf::ServiceDescriptor* serviceDesc = service->GetDescriptor();
    std::string serviceName(serviceDesc->name());
    int methodCnt = serviceDesc->method_count();
    for(int i = 0; i < methodCnt; ++i){
        const google::protobuf::MethodDescriptor* methodDesc = serviceDesc->method(i);
        std::string methodName(methodDesc->name());
        serviceInfo.methodMap[methodName] = methodDesc;
        LOG(LogLevel::INFO) << "Service: " << serviceName << ", method: " << methodName << " added to serviceMap";
    }
    serviceInfo.service = service;
    serviceMap[serviceName] = serviceInfo;
}

void MyRpcProvider::Run() {
    // 启动rpc服务，开始提供rpc远程调用服务
    std::string ip = MyRpcApplication::GetRpcServerConfig().host;
    uint16_t port = MyRpcApplication::GetRpcServerConfig().port;
    muduo::net::InetAddress listenAddr(ip, port);
    LOG(LogLevel::INFO) << "Creating MyRpcProvider server at " << ip << ":" << port;
    // 创建TcpServer对象
    muduo::net::TcpServer server(&loop, listenAddr, "MyRpcProvider");
    // 设置线程数量，默认为0表示不启用线程池
    server.setThreadNum(8);
    // 注册连接回调和消息回调
    server.setConnectionCallback(std::bind(&MyRpcProvider::OnConnection, this, std::placeholders::_1));
    server.setMessageCallback(std::bind(&MyRpcProvider::OnMessage, this, std::placeholders::_1,
        std::placeholders::_2, std::placeholders::_3));

    LOG(LogLevel::INFO) << "Starting MyRpcProvider TcpServer";
    server.start();
    LOG(LogLevel::INFO) << "Entering muduo event loop";
    loop.loop();
    LOG(LogLevel::INFO) << "Muduo event loop exited";

}

void MyRpcProvider::OnConnection(const muduo::net::TcpConnectionPtr& conn) {
    if (!conn->connected()) {
        // 断开连接，进行清理工作
        conn->shutdown();
    }
}

// 数据格式：
// old : header_size(4字节) + serviceName + methodName + args_size(4字节) + args
// RpcRequestHeader : header_size(4) + magic + version + request_id(8) + serviceName + methodName + body_len(4) + body

void MyRpcProvider::OnMessage(const muduo::net::TcpConnectionPtr& conn,
                                muduo::net::Buffer* buffer,
                                muduo::Timestamp receiveTime) {
    // 处理Rpc请求的消息
    // 1. 从buffer中读取数据，解析出Rpc请求
    // 2. 根据请求调用相应的服务方法，获取结果
    // 3. 将结果封装成Rpc响应，发送回客户端
    std::string recvBuf = buffer->retrieveAllAsString();
    LOG(LogLevel::INFO) << "Received message: " << recvBuf;
    myrpc::RpcRequestHeader requestHeader;
    uint32_t requestMagic = 0;
    uint32_t requestHeaderLen = 0;
    uint32_t requestVersion = 0;
    uint64_t requestId = 0;
    std::string serviceName;
    std::string methodName;
    std::string body;
    uint32_t bodyLen;
    // 解析Rpc请求
    if (recvBuf.size() < 4) {
        LOG(LogLevel::ERROR) << "Invalid message received: too short for header size";
        return;
    }
    recvBuf.copy(reinterpret_cast<char*>(&requestHeaderLen), sizeof(requestHeaderLen), 0);
    // 校验请求头长度是否有效
    if (recvBuf.size() < 4 + requestHeaderLen) {
        LOG(LogLevel::ERROR) << "Invalid message received: incomplete request header";
        return;
    }
    std::string requestHeaderStr(recvBuf.begin() + 4, recvBuf.begin() + 4 + requestHeaderLen);
    if(requestHeader.ParseFromString(requestHeaderStr)){
        requestMagic = requestHeader.magic();
        if(requestMagic != MyRpcApplication::GetRpcServerConfig().magic){
            LOG(LogLevel::ERROR) << "Invalid magic number: " << requestMagic;
            return;
        }
        requestVersion = requestHeader.version();
        // 校验版本号，目前只支持版本1
        switch(requestVersion){
            case 1:
                break;
            default:
                LOG(LogLevel::ERROR) << "Invalid version: " << requestVersion;
                return;
        }
        requestId = requestHeader.request_id();
        serviceName = requestHeader.service_name();
        methodName = requestHeader.method_name();
        bodyLen = requestHeader.body_len();
        // 校验请求体长度是否有效
        if (recvBuf.size() < 4 + requestHeaderLen + bodyLen) {
            LOG(LogLevel::ERROR) << "Invalid message received: incomplete request body";
            return;
        }
        body = recvBuf.substr(4 + requestHeaderLen, bodyLen);
        LOG(LogLevel::INFO) << "----------------------------------------";
        LOG(LogLevel::INFO) << "Parsed Rpc request: magic=" << requestMagic
                    << ", version=" << requestVersion
                    << ", request_id=" << requestId
                    << ", service=" << serviceName
                    << ", method=" << methodName
                    << ", body_len=" << bodyLen
                    << ", body=" << body;
        LOG(LogLevel::INFO) << "----------------------------------------";
    } else {
        LOG(LogLevel::ERROR) << "Failed to parse Rpc request";
        return;
    }

    // 2. 根据请求调用相应的服务方法，获取结果
    auto service_it = serviceMap.find(serviceName);
    if(service_it == serviceMap.end()){
        LOG(LogLevel::ERROR) << "Service not found: " << serviceName;
        return;
    }
    auto method_it = service_it->second.methodMap.find(methodName);
    if(method_it == service_it->second.methodMap.end()){
        LOG(LogLevel::ERROR) << "Method not found: " << methodName;
        return;
    }
    google::protobuf::Service* service = service_it->second.service;
    const google::protobuf::MethodDescriptor* methodDesc = method_it->second;

    // 调用服务方法
    // 1. 创建请求消息和响应消息
    std::shared_ptr<google::protobuf::Message> request = std::shared_ptr<google::protobuf::Message>(service->GetRequestPrototype(methodDesc).New());
    std::shared_ptr<google::protobuf::Message> response = std::shared_ptr<google::protobuf::Message>(service->GetResponsePrototype(methodDesc).New());
    if(!request->ParseFromString(body)){
        LOG(LogLevel::ERROR) << "Failed to parse request body";
        return;
    }

    // 3. 创建Rpc上下文
    RpcResponseContext rpcContext(conn, request, response, requestId);

    google::protobuf::Closure* done =
                        google::protobuf::NewCallback(
                                        this, &MyRpcProvider::sendRpcResponse, rpcContext);

    // 2. 在框架中根据方法描述调用RPC上发布的服务方法
    // 等价于userService.Login(request);
    service->CallMethod(methodDesc, nullptr, request.get(), response.get(), done);
}

// RpcResponseHeader : header_size(4) + magic + version + response_id(8) + errcode(4) + message_len(4) + message

void MyRpcProvider::sendRpcResponse(RpcResponseContext rpcContext) {
    std::string responseStr;

    if(rpcContext.response->SerializeToString(&responseStr)){
        myrpc::RpcResponseHeader responseHeader;
        responseHeader.set_body_len(responseStr.size());
        responseHeader.set_magic(MyRpcApplication::GetRpcServerConfig().magic);
        responseHeader.set_version(1);
        responseHeader.set_request_id(rpcContext.request_id);
        responseHeader.set_error_code(0);
        responseHeader.set_error_msg("");
        std::string headerStr = responseHeader.SerializeAsString();
        uint32_t headerLen = headerStr.size();
        std::string packet;
        packet.append(reinterpret_cast<char*>(&headerLen), 4);
        packet.append(headerStr);
        packet.append(responseStr);
        rpcContext.conn->send(packet);
    } else {
        LOG(LogLevel::ERROR) << "Failed to serialize response";
        rpcContext.conn->shutdown();
        return;
    }
    rpcContext.conn->shutdown();
   }
