#include "myrpc/consumer/MyRpcChannel.h"
#include "myrpc/application/MyRpcApplication.h"

namespace {

std::atomic<uint64_t> g_request_id{1};

struct RequestContext {
    std::string service_name;
    std::string method_name;
    uint64_t request_id;

    RequestContext(std::string service_name, std::string method_name, uint64_t request_id)
        : service_name(std::move(service_name)),
          method_name(std::move(method_name)),
          request_id(request_id) {}
};

bool Fail(google::protobuf::RpcController* controller,
          const std::string& message,
          int sockfd = -1) {
    controller->SetFailed(message);
    LOG(LogLevel::ERROR) << message;
    if (sockfd >= 0) {
        close(sockfd);
    }
    return false;
}

RequestContext BuildRequestContext(const google::protobuf::MethodDescriptor* method_desc) {
    const google::protobuf::ServiceDescriptor* service = method_desc->service();
    return RequestContext(std::string(service->name()), std::string(method_desc->name()),
                          g_request_id.fetch_add(1, std::memory_order_relaxed));
}

bool SerializeRequest(const RequestContext& ctx,
                      google::protobuf::RpcController* controller,
                      const google::protobuf::Message* request,
                      std::string* packet) {
    std::string body;
    if (!request->SerializeToString(&body)) {
        return Fail(controller, "Failed to serialize request for method: " + ctx.service_name + "." + ctx.method_name);
    }

    myrpc::RpcRequestHeader header;
    header.set_magic(MyRpcApplication::GetInstance().GetConsumerConfig().magic);
    header.set_version(1);
    header.set_request_id(ctx.request_id);
    header.set_service_name(ctx.service_name);
    header.set_method_name(ctx.method_name);
    header.set_body_len(body.size());

    std::string header_str;
    if (!header.SerializeToString(&header_str)) {
        return Fail(controller, "Failed to serialize RpcHeader for method: " + ctx.service_name + "." + ctx.method_name);
    }

    uint32_t header_len = header_str.size();
    packet->clear();
    packet->append(std::string(reinterpret_cast<char*>(&header_len), sizeof(header_len)));
    packet->append(header_str);
    packet->append(body);

    LOG(LogLevel::INFO) << "----------------------------------------";
    LOG(LogLevel::INFO) << "Serialized Rpc request: magic=" << header.magic()
                        << ", version=" << header.version()
                        << ", request_id=" << header.request_id()
                        << " service=" << ctx.service_name
                        << ", method=" << ctx.method_name
                        << ", body_len=" << body.size()
                        << ", header_size=" << header_len
                        << ", total_size=" << packet->size();
    LOG(LogLevel::INFO) << "----------------------------------------";
    return true;
}

bool ReadN(int fd, void* buf, size_t len) {
    char* ptr = static_cast<char*>(buf);
    size_t left = len;
    while (left > 0) {
        ssize_t n = recv(fd, ptr, left, 0);
        if (n < 0) {
            if (errno == EINTR) {
                continue;
            }
            return false;
        }
        if (n == 0) {
            return false;
        }
        ptr += n;
        left -= static_cast<size_t>(n);
    }
    return true;
}
bool WriteN(int fd, const void* buf, size_t len) {
    const char* ptr = static_cast<const char*>(buf);
    size_t left = len;
    while (left > 0) {
        ssize_t n = send(fd, ptr, left, 0);
        if (n < 0) {
            if (errno == EINTR) {
                continue;
            }
            return false;
        }
        if (n == 0) {
            return false;
        }
        ptr += n;
        left -= static_cast<size_t>(n);
    }
    return true;
}
bool SetSocketTimeout(int fd, int seconds) {
    timeval tv{};
    tv.tv_sec = seconds;
    tv.tv_usec = 0;
    return setsockopt(fd, SOL_SOCKET, SO_RCVTIMEO, &tv, sizeof(tv)) == 0 &&
           setsockopt(fd, SOL_SOCKET, SO_SNDTIMEO, &tv, sizeof(tv)) == 0;
}

int ConnectToServer(const std::string& service_name, google::protobuf::RpcController* controller) {
    // 从Redis注册中心获取服务端点
    RedisRegistry registry;
    std::string endpoint;
    if (!registry.DiscoverService(service_name, &endpoint)) {
        Fail(controller, "Failed to discover service: " + service_name);
        return -1;
    }
    const std::string server_ip = endpoint.substr(0, endpoint.find(':'));
    const uint16_t server_port = std::stoi(endpoint.substr(endpoint.find(':') + 1));

    int sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if (sockfd < 0) {
        Fail(controller, "Failed to create socket");
        return -1;
    }
    if (!SetSocketTimeout(sockfd, 3)) {
        Fail(controller, "Failed to set socket timeout", sockfd);
        return -1;
    }

    sockaddr_in server_addr{};
    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = inet_addr(server_ip.c_str());
    server_addr.sin_port = htons(server_port);
    if (connect(sockfd, reinterpret_cast<sockaddr*>(&server_addr), sizeof(server_addr)) < 0) {
        Fail(controller, "Failed to connect to server", sockfd);
        return -1;
    }
    return sockfd;
}

bool ReadResponseHeader(int sockfd,
                        google::protobuf::RpcController* controller,
                        myrpc::RpcResponseHeader* response_header) {
    uint32_t response_header_len = 0;
    if (!ReadN(sockfd, &response_header_len, sizeof(response_header_len))) {
        return Fail(controller, "Failed to receive response header length", sockfd);
    }

    std::string response_header_str(response_header_len, '\0');
    if (!ReadN(sockfd, response_header_str.data(), response_header_len)) {
        return Fail(controller, "Failed to receive response header", sockfd);
    }

    if (!response_header->ParseFromString(response_header_str)) {
        return Fail(controller, "Failed to parse Rpc response header", sockfd);
    }
    return true;
}

bool ValidateResponseHeader(int sockfd,
                            google::protobuf::RpcController* controller,
                            const RequestContext& ctx,
                            const myrpc::RpcResponseHeader& response_header) {
    if (response_header.magic() != MyRpcApplication::GetInstance().GetConsumerConfig().magic) {
        return Fail(controller, "Invalid magic number: " + std::to_string(response_header.magic()), sockfd);
    }
    if (response_header.version() != 1) {
        return Fail(controller, "Invalid version: " + std::to_string(response_header.version()), sockfd);
    }
    if (response_header.request_id() != ctx.request_id) {
        return Fail(controller, "RPC_REQUEST_ID_MISMATCH: invalid request_id in response", sockfd);
    }
    if (response_header.error_code() != myrpc::RPC_OK) {
        return Fail(controller, response_header.error_msg(), sockfd);
    }
    return true;
}

bool ReadResponseBody(int sockfd,
                      google::protobuf::RpcController* controller,
                      const RequestContext& ctx,
                      const myrpc::RpcResponseHeader& response_header,
                      google::protobuf::Message* response) {
    std::string body(response_header.body_len(), '\0');
    if (response_header.body_len() > 0 && !ReadN(sockfd, body.data(), body.size())) {
        return Fail(controller, "Failed to receive response body", sockfd);
    }

    LOG(LogLevel::INFO) << "----------------------------------------";
    LOG(LogLevel::INFO) << "Parsed Rpc response: magic=" << response_header.magic()
                        << ", version=" << response_header.version()
                        << ", request_id=" << response_header.request_id()
                        << ", service=" << ctx.service_name
                        << ", method=" << ctx.method_name
                        << ", body_len=" << response_header.body_len();
    LOG(LogLevel::INFO) << "----------------------------------------";

    if (!response->ParseFromString(body)) {
        return Fail(controller, "Failed to parse response from server", sockfd);
    }
    return true;
}

}  // namespace

void MyRpcChannel::CallMethod(const google::protobuf::MethodDescriptor* methodDesc,
                            google::protobuf::RpcController* controller,
                            const google::protobuf::Message* request,
                            google::protobuf::Message* response,
                            google::protobuf::Closure* done) {
    RequestContext ctx = BuildRequestContext(methodDesc);
    std::string rpc_request;
    if (!SerializeRequest(ctx, controller, request, &rpc_request)) {
        return;
    }

    const int sockfd = ConnectToServer(ctx.service_name, controller);
    if (sockfd < 0) {
        return;
    }
    if (!WriteN(sockfd, rpc_request.data(), rpc_request.size())) {
        Fail(controller, "Failed to send request to server", sockfd);
        return;
    }

    myrpc::RpcResponseHeader response_header;
    if (!ReadResponseHeader(sockfd, controller, &response_header)) {
        return;
    }
    if (!ValidateResponseHeader(sockfd, controller, ctx, response_header)) {
        return;
    }
    if (!ReadResponseBody(sockfd, controller, ctx, response_header, response)) {
        return;
    }

    close(sockfd);
}
