#include "myrpc/registry/RedisRegistry.h"

#include <hiredis/hiredis.h>

#include "log.h"
#include "myrpc/application/MyRpcApplication.h"

namespace {

std::string ServiceKey(const std::string& service_name) {
    return "myrpc:service:" + service_name;
}

}  // namespace

RedisRegistry::RedisRegistry() : context_(nullptr) {}

RedisRegistry::~RedisRegistry() {
    Disconnect();
}

bool RedisRegistry::Connect() {
    if (context_ != nullptr) {
        LOG(LogLevel::INFO) << "Redis context already connected";
        return true;
    }
    const RedisConfig& config = MyRpcApplication::GetRedisConfig();
    timeval timeout{};
    timeout.tv_sec = 3;
    timeout.tv_usec = 0;
    // 连接Redis服务器
    context_ = redisConnectWithTimeout(config.host.c_str(), config.port, timeout);
    if (context_ == nullptr || context_->err) {
        if (context_ != nullptr) {
            LOG(LogLevel::ERROR) << "Failed to connect to redis: " << context_->errstr;
            redisFree(context_);
            context_ = nullptr;
        } else {
            LOG(LogLevel::ERROR) << "Failed to allocate redis context";
        }
        return false;
    }
    // 认证Redis服务器
    if (!config.password.empty()) {
        redisReply* reply = static_cast<redisReply*>(redisCommand(context_, "AUTH %s", config.password.c_str()));
        if (reply == nullptr || context_->err) {
            LOG(LogLevel::ERROR) << "Redis AUTH failed";
            if (reply != nullptr) {
                freeReplyObject(reply);
            }
            Disconnect();
            return false;
        }
        freeReplyObject(reply);
    }
    // 选择Redis数据库
    if (config.db != 0) {
        redisReply* reply = static_cast<redisReply*>(redisCommand(context_, "SELECT %d", config.db));
        if (reply == nullptr || context_->err) {
            LOG(LogLevel::ERROR) << "Redis SELECT failed";
            if (reply != nullptr) {
                freeReplyObject(reply);
            }
            Disconnect();
            return false;
        }
        freeReplyObject(reply);
    }

    return true;
}

void RedisRegistry::Disconnect() {
    if (context_ != nullptr) {
        LOG(LogLevel::INFO) << "Disconnecting from Redis server";
        redisFree(context_);
        context_ = nullptr;
    }
}

bool RedisRegistry::RegisterService(const std::string& service_name, const std::string& endpoint) {
    LOG(LogLevel::INFO) << "Registering service: " << service_name << " -> " << endpoint;
    if (!Connect()) {
        return false;
    }
    // 注册服务到Redis服务器
    redisReply* reply = static_cast<redisReply*>(
        redisCommand(context_, "SET %s %s", ServiceKey(service_name).c_str(), endpoint.c_str()));
    if (reply == nullptr || context_->err) {
        LOG(LogLevel::ERROR) << "Redis register service failed: " << service_name;
        if (reply != nullptr) {
            freeReplyObject(reply);
        }
        return false;
    }

    freeReplyObject(reply);
    return true;
}

bool RedisRegistry::DiscoverService(const std::string& service_name, std::string* endpoint) {
    LOG(LogLevel::INFO) << "Discovering service: " << service_name;
    if (!Connect()) {
        return false;
    }
    // 从Redis服务器获取服务端点
    redisReply* reply = static_cast<redisReply*>(
        redisCommand(context_, "GET %s", ServiceKey(service_name).c_str()));
    if (reply == nullptr || context_->err) {
        LOG(LogLevel::ERROR) << "Redis discover service failed: " << service_name;
        if (reply != nullptr) {
            freeReplyObject(reply);
        }
        return false;
    }
    // 检查Redis回复是否为字符串类型
    if (reply->type != REDIS_REPLY_STRING) {
        freeReplyObject(reply);
        return false;
    }
    // 复制服务端点到输出参数
    *endpoint = reply->str;
    freeReplyObject(reply);
    return true;
}
