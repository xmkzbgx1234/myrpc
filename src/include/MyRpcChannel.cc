#include "MyRpcChannel.h"
#include "rpcHeader.pb.h"
#include "log.h"
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <sys/types.h>
#include "MyRpcApplication.h"

void MyRpcChannel::CallMethod(const google::protobuf::MethodDescriptor* methodDesc,
                            google::protobuf::RpcController* controller,
                            const google::protobuf::Message* request,
                            google::protobuf::Message* response,
                            google::protobuf::Closure* done) {
    // 1. 将method、request等信息进行序列化，封装成Rpc请求的格式，进行网络发送
    // 2. 在框架中根据方法描述调用RPC上发布的服务方法，获取response
    // 3. 将response进行序列化，通过网络发送给rpc调用方
    const google::protobuf::ServiceDescriptor* service = methodDesc->service();
    std::string service_name(service->name());
    std::string method_name(methodDesc->name());
    int args_size = 0;
    std::string args;
    if(request->SerializeToString(&args)){
        args_size = args.size();
    } else {
        // 序列化失败，进行错误处理
        LOG(LogLevel::ERROR) << "Failed to serialize request for method: " << service_name << "." << method_name;
        return;
    }
    // 2. 封装Rpc请求的格式
    // 计算header的size
    uint32_t header_size = 0;
    myrpc::myRpcHeader header;
    header.set_service_name(service_name);
    header.set_method_name(method_name);
    header.set_args_size(args_size);
    std::string header_str;
    if(header.SerializeToString(&header_str)){
        header_size = header_str.size();
    } else {
        // 序列化失败，进行错误处理
        LOG(LogLevel::ERROR) << "Failed to serialize RpcHeader for method: " << service_name << "." << method_name;
        return;
    }
    // 3. 封装序列化的Rpc请求的格式
    std::string rpc_request;
    rpc_request.append(std::string((char*)&header_size, 4)); // header_size占4字节
    rpc_request.append(header_str); // header_str占header_size字节
    rpc_request.append(args); // args占args_size字节
    LOG(LogLevel::INFO) << "----------------------------------------";
    LOG(LogLevel::INFO) << "Serialized Rpc request: service=" << service_name
                << ", method=" << method_name
                << ", args_size=" << args_size
                << ", header_size=" << header_size
                << ", total_size=" << rpc_request.size()
                << ", request=" << rpc_request;
    LOG(LogLevel::INFO) << "----------------------------------------";

    std::string server_ip = MyRpcApplication::GetInstance().GetRpcServerConfig().host;
    uint16_t server_port = MyRpcApplication::GetInstance().GetRpcServerConfig().port;

    // 使用TCP简易测试，发送Rpc请求
    int sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if(sockfd < 0) {
        LOG(LogLevel::ERROR) << "Failed to create socket";
        return;
    }
    // 连接服务器
    struct sockaddr_in server_addr;
    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = inet_addr(server_ip.c_str());
    server_addr.sin_port = htons(server_port);
    if(connect(sockfd, (struct sockaddr*)&server_addr, sizeof(server_addr)) < 0) {
        LOG(LogLevel::ERROR) << "Failed to connect to server";
        close(sockfd);
        return;
    }
    // 发送Rpc请求
    if(send(sockfd, rpc_request.c_str(), rpc_request.size(), 0) < 0) {
        LOG(LogLevel::ERROR) << "Failed to send request to server";
        close(sockfd);
        return;
    }

    // 接收Rpc响应
    char buf[1024];
    int bytes_received = recv(sockfd, buf, sizeof(buf), 0);
    if(bytes_received <= 0) {
        LOG(LogLevel::ERROR) << "Failed to receive response from server";
        close(sockfd);
        return;
    }
    // 解析Rpc响应
    std::string response_str(buf, bytes_received);
    if(!response->ParseFromString(response_str)) {
        LOG(LogLevel::ERROR) << "Failed to parse response from server";
        close(sockfd);
        return;
    }
    // 关闭socket
    close(sockfd);
}
