#include "user.pb.h"
#include "MyRpcApplication.h"
#include "MyRpcProvider.h"
#include "MyRpcChannel.h"
#include <iostream>

int main(int argc, char** argv) {

    MyRpcApplication::GetInstance().Init(argc, argv);

    fixbug::UserServiceRpc_Stub stub(new MyRpcChannel());

    fixbug::RegisterRequest registerRequest;
    registerRequest.set_userid(1);
    registerRequest.set_name("xmk");
    registerRequest.set_pwd("123456");

    fixbug::RegisterResponse registerResponse;
    stub.Register(nullptr, &registerRequest, &registerResponse, nullptr);
    if(registerResponse.rescode().errcode() == 0) {
        std::cout << "register success" << registerResponse.success() << std::endl;
    } else {
        std::cout << "register failed" << registerResponse.rescode().message() << std::endl;
    }

    fixbug::LoginRequest loginRequest;
    loginRequest.set_name("xmk");
    loginRequest.set_pwd("123456");

    fixbug::LoginResponse response;

    // myrpc框架调用路径
    // stub => MyRpcChannel => RpcChannel::CallMethod => MyRpcProvider::OnMessage => MyRpcProvider::CallMethod
    stub.Login(nullptr, &loginRequest, &response, nullptr);

    if(response.rescode().errcode() == 0) {
        std::cout << "login success" << response.success() << std::endl;
    } else {
        std::cout << "login failed" << response.rescode().message() << std::endl;
    }

    return 0;
}
