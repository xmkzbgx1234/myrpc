#include "log.h"
#include "myrpc/application/MyRpcApplication.h"
#include "config.h"
#include <unistd.h>

ProviderConfig MyRpcApplication::rpcProviderConfig;
ConsumerConfig MyRpcApplication::consumerConfig;
RedisConfig MyRpcApplication::redisConfig;
AppRole MyRpcApplication::role = AppRole::None;

void MyRpcApplication::Init(int argc, char** argv) {
    // 解析命令行参数，进行一些初始化工作
    LOG(LogLevel::INFO) << "MyRpcApplication initialized with " << argc << " arguments.";
    if(argc < 4){
        LOG(LogLevel::ERROR) << "Usage: " << argv[0] << " <config_file> -r <provider|consumer>";
        exit(EXIT_FAILURE);
    }
    int c = 0;
    std::string config_file;
    std::string role_str;
    while ((c = getopt(argc, argv, "i:r:")) != -1) {
        switch (c) {
        case 'i':
            config_file = optarg;
            break;
        case 'r':
            role_str = optarg;
            if(role_str == "provider"){
                role = AppRole::Provider;
            } else if(role_str == "consumer"){
                role = AppRole::Consumer;
            } else {
                LOG(LogLevel::ERROR) << "Invalid role: " << role_str;
                LOG(LogLevel::ERROR) << "Usage: " << argv[0] << " <config_file> -r <provider|consumer>";
                exit(EXIT_FAILURE);
            }
            break;
        default:
            LOG(LogLevel::ERROR) << "Unknown option: " << char(optopt);
            LOG(LogLevel::ERROR) << "Usage: " << argv[0] << " <config_file> -r <provider|consumer>";
            exit(EXIT_FAILURE);
        }
    }

    // 加载配置文件，进行相应的初始化
    LOG(LogLevel::INFO) << "Configuration file: " << config_file;
    Config config;
    try{
        config.load(config_file);
        if(role == AppRole::Provider){
            rpcProviderConfig.loadFrom(config);
            LOG(LogLevel::INFO) << "RPC Provider will start at " << rpcProviderConfig.host << ":" << rpcProviderConfig.port;
        } else if(role == AppRole::Consumer){
            consumerConfig.loadFrom(config);
        }
        redisConfig.loadFrom(config);
        LOG(LogLevel::INFO) << "Redis Server will start at " << redisConfig.host << ":" << redisConfig.port;
    } catch (const ConfigException& ex){
        LOG(LogLevel::ERROR) << "Configuration error: " << ex.what();
        exit(EXIT_FAILURE);
    }
}

MyRpcApplication& MyRpcApplication::GetInstance() {
    static MyRpcApplication app;
    return app;
}

ProviderConfig& MyRpcApplication::GetRpcProviderConfig() {
    return rpcProviderConfig;
}

ConsumerConfig& MyRpcApplication::GetConsumerConfig() {
    return consumerConfig;
}
RedisConfig& MyRpcApplication::GetRedisConfig() {
    return redisConfig;
}

AppRole& MyRpcApplication::GetRole() {
    return role;
}
