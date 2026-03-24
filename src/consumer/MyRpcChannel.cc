#include "myrpc/consumer/MyRpcChannel.h"
#include "rpcHeader.pb.h"
#include "log.h"
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <sys/types.h>
#include "myrpc/application/MyRpcApplication.h"

static std::atomic<uint64_t> g_request_id{1};

void MyRpcChannel::CallMethod(const google::protobuf::MethodDescriptor* methodDesc,
                            google::protobuf::RpcController* controller,
                            const google::protobuf::Message* request,
                            google::protobuf::Message* response,
                            google::protobuf::Closure* done) {
    // 1. 将method、request等信息进行序列化，封装成Rpc请求的格式，进行网络发送
    // 2. 在框架中根据方法描述调用RPC上发布的服务方法，获取response
    // 3. 将response进行序列化，通过网络发送给rpc调用方
    const google::protobuf::ServiceDescriptor* service = methodDesc->service();
    std::string serviceName(service->name());
    std::string methodName(methodDesc->name());
    uint32_t args_size = 0;
    std::string args;
    if(request->SerializeToString(&args)){
        args_size = args.size();
    } else {
        // 序列化失败，进行错误处理
        controller->SetFailed("Failed to serialize request for method: " + serviceName + "." + methodName);
        LOG(LogLevel::ERROR) << "Failed to serialize request for method: " << serviceName << "." << methodName;
        return;
    }
    // 封装Rpc请求的格式
    // 计算header的size
    uint32_t requestHeaderLen = 0;
    myrpc::RpcRequestHeader header;
    header.set_magic(MyRpcApplication::GetInstance().GetRpcServerConfig().magic);
    header.set_version(1);
    uint64_t requestId = g_request_id.fetch_add(1, std::memory_order_relaxed);
    header.set_request_id(requestId);
    header.set_service_name(serviceName);
    header.set_method_name(methodName);
    header.set_body_len(args_size);
    std::string header_str;
    if(header.SerializeToString(&header_str)){
        requestHeaderLen = header_str.size();
    } else {
        // 序列化失败，进行错误处理
        controller->SetFailed("Failed to serialize RpcHeader for method: " + serviceName + "." + methodName);
        LOG(LogLevel::ERROR) << "Failed to serialize RpcHeader for method: " << serviceName << "." << methodName;
        return;
    }
    // 3. 封装序列化的Rpc请求的格式
    std::string rpc_request;
    rpc_request.append(std::string((char*)&requestHeaderLen, 4)); // header_size占4字节
    rpc_request.append(header_str); // header_str占header_size字节
    rpc_request.append(args); // args占args_size字节
    LOG(LogLevel::INFO) << "----------------------------------------";
    LOG(LogLevel::INFO) << "Serialized Rpc request: magic=" << header.magic()
                << ", version=" << header.version()
                << ", request_id=" << header.request_id()
                << " service=" << serviceName
                << ", method=" << methodName
                << ", args_size=" << args_size
                << ", header_size=" << requestHeaderLen << " bytes"
                << ", total_size=" << rpc_request.size()
                << ", request=" << rpc_request;
    LOG(LogLevel::INFO) << "----------------------------------------";

    std::string server_ip = MyRpcApplication::GetInstance().GetRpcServerConfig().host;
    uint16_t server_port = MyRpcApplication::GetInstance().GetRpcServerConfig().port;

    // 使用TCP简易测试，发送Rpc请求
    int sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if(sockfd < 0) {
        controller->SetFailed("Failed to create socket");
        LOG(LogLevel::ERROR) << "Failed to create socket";
        return;
    }
    // 连接服务器
    struct sockaddr_in server_addr;
    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = inet_addr(server_ip.c_str());
    server_addr.sin_port = htons(server_port);
    if(connect(sockfd, (struct sockaddr*)&server_addr, sizeof(server_addr)) < 0) {
        controller->SetFailed("Failed to connect to server");
        LOG(LogLevel::ERROR) << "Failed to connect to server";
        close(sockfd);
        return;
    }
    // 发送Rpc请求
    if(send(sockfd, rpc_request.c_str(), rpc_request.size(), 0) < 0) {
        controller->SetFailed("Failed to send request to server");
        LOG(LogLevel::ERROR) << "Failed to send request to server";
        close(sockfd);
        return;
    }

    // 接收Rpc响应
    char buf[1024];
    int bytes_received = recv(sockfd, buf, sizeof(buf), 0);
    if(bytes_received <= 0) {
        controller->SetFailed("Failed to receive response from server");
        LOG(LogLevel::ERROR) << "Failed to receive response from server";
        close(sockfd);
        return;
    }
    std::string recvBuf(buf, bytes_received);
    // 解析Rpc响应
    myrpc::RpcResponseHeader responseHeader;
    uint32_t responseHeaderLen = 0;
    uint32_t responseMagic = 0;
    uint32_t responseVersion = 0;
    uint64_t responseRequestId = 0;
    uint32_t responseErrorCode = 0;
    std::string responseErrorMsg;
    uint32_t bodyLen;
    std::string body;
    // 解析Rpc响应的header_size
    if (recvBuf.size() < 4) {
        LOG(LogLevel::ERROR) << "Invalid message received: too short for header size";
        return;
    }
    recvBuf.copy(reinterpret_cast<char*>(&responseHeaderLen), sizeof(responseHeaderLen), 0);
    // 校验响应头长度是否有效
    if (recvBuf.size() < 4 + responseHeaderLen) {
        LOG(LogLevel::ERROR) << "Invalid message received: incomplete response header";
        return;
    }
    std::string responseHeaderStr(recvBuf.begin() + 4, recvBuf.begin() + 4 + responseHeaderLen);
    if(responseHeader.ParseFromString(responseHeaderStr)){
        responseMagic = responseHeader.magic();
        if(responseMagic != MyRpcApplication::GetRpcServerConfig().magic){
            LOG(LogLevel::ERROR) << "Invalid magic number: " << responseMagic;
            return;
        }
        responseVersion = responseHeader.version();
        // 校验版本号，目前只支持版本1
        switch(responseVersion){
            case 1:
                break;
            default:
                LOG(LogLevel::ERROR) << "Invalid version: " << responseVersion;
                return;
        }
        responseRequestId = responseHeader.request_id();
        if(responseRequestId != header.request_id()){
            LOG(LogLevel::ERROR) << "Invalid request_id: " << responseRequestId;
            return;
        }
        responseErrorCode = responseHeader.error_code();
        responseErrorMsg = responseHeader.error_msg();
        if (responseErrorCode != 0) {
            controller->SetFailed(responseErrorMsg);
            LOG(LogLevel::ERROR) << "RPC framework error: code=" << responseErrorCode
                                << ", msg=" << responseErrorMsg;
            close(sockfd);
            return;
        }
        bodyLen = responseHeader.body_len();
        // 校验响应体长度是否有效
        if (recvBuf.size() < 4 + responseHeaderLen + bodyLen) {
            LOG(LogLevel::ERROR) << "Invalid message received: incomplete response body";
            return;
        }
        body = recvBuf.substr(4 + responseHeaderLen, bodyLen);
        LOG(LogLevel::INFO) << "----------------------------------------";
        LOG(LogLevel::INFO) << "Parsed Rpc response: magic=" << responseMagic
                    << ", version=" << responseVersion
                    << ", request_id=" << responseRequestId
                    << ", service=" << serviceName
                    << ", method=" << methodName
                    << ", body_len=" << bodyLen
                    << ", body=" << body;
        LOG(LogLevel::INFO) << "----------------------------------------";
    } else {
        LOG(LogLevel::ERROR) << "Failed to parse Rpc response";
        return;
    }

    if(!response->ParseFromString(body)) {
        controller->SetFailed("Failed to parse response from server");
        LOG(LogLevel::ERROR) << "Failed to parse response from server";
        close(sockfd);
        return;
    }

    // 关闭socket
    close(sockfd);
}
