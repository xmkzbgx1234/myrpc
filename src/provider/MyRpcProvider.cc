#include "myrpc/provider/MyRpcProvider.h"
#include "myrpc/application/MyRpcApplication.h"

namespace {

struct ParsedRequest {
    uint64_t request_id = 0;
    std::string service_name;
    std::string method_name;
    std::string body;
};
}  // namespace

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
    std::string ip = MyRpcApplication::GetRpcProviderConfig().host;
    uint16_t port = MyRpcApplication::GetRpcProviderConfig().port;
    muduo::net::InetAddress listenAddr(ip, port);
    LOG(LogLevel::INFO) << "Creating MyRpcProvider server at " << ip << ":" << port;

    RedisRegistry registry;
    std::string endpoint = ip + ":" + std::to_string(port);
    for (const auto& [service_name, service_info] : serviceMap) {
        registry.RegisterService(service_name, endpoint);
    }

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
    // 循环处理多个请求包
    while (true) {
        if (buffer->readableBytes() < sizeof(uint32_t)) {
            return;
        }
        const char* data = buffer->peek();
        uint32_t header_len = 0;
        memcpy(&header_len, data, sizeof(uint32_t));
        if (buffer->readableBytes() < sizeof(uint32_t) + header_len) {
            return;
        }
        myrpc::RpcRequestHeader request_header;
        std::string header_str(data + sizeof(uint32_t), header_len);
        if (!request_header.ParseFromString(header_str)) {
            LOG(LogLevel::ERROR) << "Failed to parse Rpc request header";
            buffer->retrieveAll();
            return;
        }
        uint32_t body_len = request_header.body_len();
        uint32_t total_len = sizeof(uint32_t) + header_len + body_len;
        if (buffer->readableBytes() < total_len) {
            return;
        }
        std::string body(data + sizeof(uint32_t) + header_len, body_len);
        buffer->retrieve(total_len);
        // 这里处理一个完整请求
        HandleRequest(conn, request_header, body, MyRpcApplication::GetRpcProviderConfig().magic);
    }
}

void MyRpcProvider::HandleRequest(const muduo::net::TcpConnectionPtr& conn,
                  const myrpc::RpcRequestHeader& request_header,
                  const std::string& body, uint32_t expected_magic) {
    LOG(LogLevel::INFO) << "Received request: " << request_header.DebugString() << ", body=" << body;
    // 校验requestHeader
    if(request_header.magic() != expected_magic) {
        sendErrorResponse(conn, request_header.request_id(), myrpc::RPC_INVALID_MAGIC, "Invalid magic number");
        LOG(LogLevel::ERROR) << "Invalid magic number: " << request_header.magic();
        return;
    }
    if(request_header.version() != 1) {
        sendErrorResponse(conn, request_header.request_id(), myrpc::RPC_UNSUPPORTED_VERSION, "Unsupported version");
        LOG(LogLevel::ERROR) << "Unsupported version: " << request_header.version();
        return;
    }

    // 查找服务对象和方法
    auto service_it = serviceMap.find(request_header.service_name());
    if(service_it == serviceMap.end()){
        // 服务对象不存在
        sendErrorResponse(conn, request_header.request_id(), myrpc::RPC_SERVICE_NOT_FOUND, "Service not found");
        LOG(LogLevel::ERROR) << "Service not found: " << request_header.service_name();
        return;
    }
    auto method_it = service_it->second.methodMap.find(request_header.method_name());
    if(method_it == service_it->second.methodMap.end()){
        // 方法不存在
        sendErrorResponse(conn, request_header.request_id(), myrpc::RPC_METHOD_NOT_FOUND, "Method not found");
        LOG(LogLevel::ERROR) << "Method not found: " << request_header.method_name();
        return;
    }
    google::protobuf::Service* service = service_it->second.service;
    const google::protobuf::MethodDescriptor* methodDesc = method_it->second;
    std::shared_ptr<google::protobuf::Message> request = std::shared_ptr<google::protobuf::Message>(service->GetRequestPrototype(methodDesc).New());
    std::shared_ptr<google::protobuf::Message> response = std::shared_ptr<google::protobuf::Message>(service->GetResponsePrototype(methodDesc).New());
    if(!request->ParseFromString(body)){
        sendErrorResponse(conn, request_header.request_id(), myrpc::RPC_REQUEST_PARSE_ERROR,
                          "Failed to parse request body");
        LOG(LogLevel::ERROR) << "Failed to parse request body";
        return;
    }

    MyRpcProvider::RpcResponseContext rpcContext(conn, request, response, request_header.request_id());
    google::protobuf::Closure* done =
                            google::protobuf::NewCallback(this, &MyRpcProvider::sendRpcResponse, rpcContext);
    service->CallMethod(methodDesc, nullptr, request.get(), response.get(), done);
}

// RpcResponseHeader : header_size(4) + magic + version + response_id(8) + errcode(4) + message_len(4) + message
// 发送正常Rpc响应
void MyRpcProvider::sendRpcResponse(RpcResponseContext rpcContext) {
    std::string responseStr;
    if (!rpcContext.response->SerializeToString(&responseStr)) {
        // 序列化响应体失败，发送错误响应
        sendErrorResponse(rpcContext.conn,
                          rpcContext.request_id,
                          myrpc::RPC_RESPONSE_SERIALIZE_ERROR,
                          "Failed to serialize response");
        return;
    }
    myrpc::RpcResponseHeader responseHeader;
    responseHeader.set_magic(MyRpcApplication::GetRpcProviderConfig().magic);
    responseHeader.set_version(1);
    responseHeader.set_request_id(rpcContext.request_id);
    responseHeader.set_body_len(responseStr.size());
    responseHeader.set_error_code(myrpc::RPC_OK);
    responseHeader.set_error_msg("");
    std::string headerStr;
    if (!responseHeader.SerializeToString(&headerStr)) {
        // 序列化响应头失败，发送错误响应
        sendErrorResponse(rpcContext.conn,
                          rpcContext.request_id,
                          myrpc::RPC_INTERNAL_ERROR,
                          "Failed to serialize response header");
        return;
    }
    uint32_t headerLen = headerStr.size();
    std::string packet;
    packet.append(reinterpret_cast<char*>(&headerLen), 4);
    packet.append(headerStr);
    packet.append(responseStr);
    rpcContext.conn->send(packet);
    rpcContext.conn->shutdown();
}

// 发送错误响应
void MyRpcProvider::sendErrorResponse(const muduo::net::TcpConnectionPtr& conn,
                                      uint64_t requestId,
                                      int errorCode,
                                      const std::string& errorMsg) {
    myrpc::RpcResponseHeader responseHeader;
    responseHeader.set_magic(MyRpcApplication::GetRpcProviderConfig().magic);
    responseHeader.set_version(1);
    responseHeader.set_request_id(requestId);
    responseHeader.set_body_len(0);
    responseHeader.set_error_code(errorCode);
    responseHeader.set_error_msg(errorMsg);
    std::string headerStr;
    if (!responseHeader.SerializeToString(&headerStr)) {
        LOG(LogLevel::ERROR) << "Failed to serialize error response header";
        conn->shutdown();
        return;
    }
    uint32_t headerLen = headerStr.size();
    std::string packet;
    packet.append(reinterpret_cast<char*>(&headerLen), 4);
    packet.append(headerStr);
    conn->send(packet);
    conn->shutdown();
}
