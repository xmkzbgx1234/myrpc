#include "log.h"
#include "MyRpcApplication.h"
#include "config.h"
#include <unistd.h>

ServerConfig MyRpcApplication::rpcServerConfig;

void ShowArgsHelp(){
    std::cout << "Format: command -i <configfile>" << "/n";
}

void MyRpcApplication::Init(int argc, char** argv) {
    // 解析命令行参数，进行一些初始化工作
    LOG(LogLevel::INFO) << "MyRpcApplication initialized with " << argc << " arguments.";
    if(argc < 2){
        ShowArgsHelp();
        LOG(LogLevel::ERROR) << "Usage: " << argv[0] << " <config_file>";
        exit(EXIT_FAILURE);
    }
    int c = 0;
    std::string config_file;
    while ((c = getopt(argc, argv, "i:")) != -1)
    {
        switch (c)
        {
        case 'i':
            config_file = optarg;
            break;
        case '?':
            LOG(LogLevel::ERROR) << "Unknown option: " << char(optopt);
            ShowArgsHelp();
            exit(EXIT_FAILURE);
        case ':':
            LOG(LogLevel::ERROR) << "Option requires an argument: " << char(optopt);
            ShowArgsHelp();
            exit(EXIT_FAILURE);
        default:
            break;
        }
    }
    // 加载配置文件，进行相应的初始化
    LOG(LogLevel::INFO) << "Configuration file: " << config_file;
    Config config;
    try{
        config.load(config_file);
        rpcServerConfig.loadFrom(config);
        LOG(LogLevel::INFO) << "RPC Server will start at " << rpcServerConfig.host << ":" << rpcServerConfig.port;
    } catch (const ConfigException& ex){
        LOG(LogLevel::ERROR) << "Configuration error: " << ex.what();
        exit(EXIT_FAILURE);
    }
}

MyRpcApplication& MyRpcApplication::GetInstance() {
    static MyRpcApplication app;
    return app;
}

ServerConfig& MyRpcApplication::GetRpcServerConfig() {
    return rpcServerConfig;
}
