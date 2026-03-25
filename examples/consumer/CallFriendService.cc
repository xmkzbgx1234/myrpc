#include <iostream>

#include "user.pb.h"
#include "myrpc/application/MyRpcApplication.h"
#include "myrpc/consumer/MyRpcChannel.h"
#include "myrpc/consumer/MyRpcController.h"

int main(int argc, char** argv) {
    MyRpcApplication::GetInstance().Init(argc, argv);

    fixbug::FriendServiceRpc_Stub stub(new MyRpcChannel());
    MyRpcController controller;

    fixbug::FriendListRequest request;
    request.set_userid(10001);

    fixbug::FriendListResponse response;
    stub.GetFriendList(&controller, &request, &response, nullptr);

    if (controller.Failed()) {
        std::cout << "rpc failed: " << controller.ErrorText() << std::endl;
        return 1;
    }

    if (response.rescode().errcode() != 0) {
        std::cout << "business failed: " << response.rescode().message() << std::endl;
        return 1;
    }

    std::cout << "friend list:" << std::endl;
    for (int i = 0; i < response.friends_size(); ++i) {
        std::cout << "- " << response.friends(i) << std::endl;
    }
    return 0;
}
