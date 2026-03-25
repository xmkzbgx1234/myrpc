#pragma once

#include <memory>
#include <string>

struct redisContext;

class RedisRegistry {
public:
    RedisRegistry();
    ~RedisRegistry();

    RedisRegistry(const RedisRegistry&) = delete;
    RedisRegistry& operator=(const RedisRegistry&) = delete;

    bool Connect();
    void Disconnect();

    bool RegisterService(const std::string& service_name, const std::string& endpoint);
    bool DiscoverService(const std::string& service_name, std::string* endpoint);

private:
    redisContext* context_;
};
