#include "myrpc/application/MyRpcApplication.h"
#include "myrpc/consumer/MyRpcChannel.h"
#include "myrpc/consumer/MyRpcController.h"
#include "user.pb.h"

int main(int argc, char** argv) {

    MyRpcApplication::GetInstance().Init(argc, argv);
    fixbug::UserServiceRpc_Stub userServiceStub(new MyRpcChannel());

    MyRpcController controller;

    fixbug::RegisterRequest registerRequest;
    registerRequest.set_userid(1);
    registerRequest.set_name("xmk");
    registerRequest.set_pwd("123456");

    fixbug::RegisterResponse registerResponse;
    userServiceStub.Register(&controller, &registerRequest, &registerResponse, nullptr);
    if(!controller.Failed() && registerResponse.rescode().errcode() == 0) {
        std::cout << "register success" << registerResponse.success() << std::endl;
    } else {
        std::cout << controller.ErrorText() << std::endl;
        std::cout << "register failed" << registerResponse.rescode().message() << std::endl;
    }

    fixbug::LoginRequest loginRequest;
    loginRequest.set_name("xmk");
    loginRequest.set_pwd("123456");

    fixbug::LoginResponse response;
    controller.Reset();

    // myrpc框架调用路径
    // stub => MyRpcChannel => RpcChannel::CallMethod => MyRpcProvider::OnMessage => MyRpcProvider::CallMethod
    userServiceStub.Login(&controller, &loginRequest, &response, nullptr);

    if(!controller.Failed() && response.rescode().errcode() == 0) {
        std::cout << "login success" << response.success() << std::endl;
    } else {
        std::cout << controller.ErrorText() << std::endl;
        std::cout << "login failed" << response.rescode().message() << std::endl;
    }

    fixbug::FriendServiceRpc_Stub friendServiceStub(new MyRpcChannel());
    fixbug::FriendListRequest friendListRequest;
    friendListRequest.set_userid(1);
    fixbug::FriendListResponse friendListResponse;
    friendServiceStub.GetFriendList(&controller, &friendListRequest, &friendListResponse, nullptr);
    if(!controller.Failed() && friendListResponse.rescode().errcode() == 0) {
        std::cout << "friend list:" << std::endl;
        for (int i = 0; i < friendListResponse.friends_size(); ++i) {
            std::cout << "- " << friendListResponse.friends(i) << std::endl;
        }
    } else {
        std::cout << controller.ErrorText() << std::endl;
        std::cout << "friend list failed" << friendListResponse.rescode().message() << std::endl;
    }

    return 0;
}
