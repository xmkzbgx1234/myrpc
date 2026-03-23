# pragma once
#include "config.h"
class MyRpcApplication {

public:
    static void Init(int argc, char** argv);
    static MyRpcApplication& GetInstance();
    static ServerConfig& GetRpcServerConfig();
    static void Destroy();

private:
    MyRpcApplication() = default;
    // 禁止拷贝构造和移动构造
    MyRpcApplication(const MyRpcApplication&) = delete;
    MyRpcApplication& operator=(const MyRpcApplication&) = delete;
    MyRpcApplication(MyRpcApplication&&) = delete;

    static ServerConfig rpcServerConfig;

};