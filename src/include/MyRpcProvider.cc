#include "MyRpcProvider.h"
#include "MyRpcApplication.h"
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
// header_size(4字节) + serviceName + methodName + args_size(4字节) + args

void MyRpcProvider::OnMessage(const muduo::net::TcpConnectionPtr& conn,
                                muduo::net::Buffer* buffer,
                                muduo::Timestamp receiveTime) {
    // 处理Rpc请求的消息
    // 1. 从buffer中读取数据，解析出Rpc请求
    // 2. 根据请求调用相应的服务方法，获取结果
    // 3. 将结果封装成Rpc响应，发送回客户端
    std::string recvBuf = buffer->retrieveAllAsString();
    LOG(LogLevel::INFO) << "Received message: " << recvBuf;
    myrpc::myRpcHeader header;
    std::string serviceName;
    std::string methodName;
    std::string args;
    uint32_t argsSize;
    // 解析Rpc请求
    uint32_t headerSize = 0;
    if (recvBuf.size() < 4) {
        LOG(LogLevel::ERROR) << "Invalid message received: too short for header size";
    }
    recvBuf.copy(reinterpret_cast<char*>(&headerSize), sizeof(headerSize), 0);
    std::string headerStr(recvBuf.begin() + 4, recvBuf.begin() + 4 + headerSize);
    if(header.ParseFromString(headerStr)){
        serviceName = header.service_name();
        methodName = header.method_name();
        argsSize = header.args_size();
        args = recvBuf.substr(4 + headerSize, argsSize);
        LOG(LogLevel::INFO) << "----------------------------------------";
        LOG(LogLevel::INFO) << "Parsed Rpc request: service=" << header.service_name()
                    << ", method=" << header.method_name()
                    << ", args_size=" << header.args_size()
                    << ", args=" << args;
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
    google::protobuf::Message* request = service->GetRequestPrototype(methodDesc).New();
    google::protobuf::Message* response = service->GetResponsePrototype(methodDesc).New();
    if(!request->ParseFromString(args)){
        LOG(LogLevel::ERROR) << "Failed to parse request args";
        return;
    }

    google::protobuf::Closure* done =
        google::protobuf::NewCallback<MyRpcProvider, muduo::net::TcpConnectionPtr, google::protobuf::Message*>(
            this,
            &MyRpcProvider::sendRpcResponse,
            conn,
            response);

    // 2. 在框架中根据方法描述调用RPC上发布的服务方法
    // 等价于userService.Login(request);
    service->CallMethod(methodDesc, nullptr, request, response, done);

}

void MyRpcProvider::sendRpcResponse(muduo::net::TcpConnectionPtr conn, google::protobuf::Message* response) {
    std::string responseStr;
    if(response->SerializeToString(&responseStr)){
        // 序列化成功，发送响应到客户端
        conn->send(responseStr);
    } else {
        LOG(LogLevel::ERROR) << "Failed to serialize response";
    }
    conn->shutdown();
}
