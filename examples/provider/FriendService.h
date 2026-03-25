#pragma once

#include <iostream>
#include <string>
#include <vector>

#include "user.pb.h"
#include "myrpc/application/MyRpcApplication.h"
#include "myrpc/provider/MyRpcProvider.h"

class FriendService : public fixbug::FriendServiceRpc {
public:
    std::vector<std::string> GetFriendList(int userid) {
        std::cout << "GetFriendList: userid=" << userid << std::endl;
        return {"alice", "bob", "charlie"};
    }

    void GetFriendList(::google::protobuf::RpcController* controller,
                       const ::fixbug::FriendListRequest* request,
                       ::fixbug::FriendListResponse* response,
                       ::google::protobuf::Closure* done) override {
        std::vector<std::string> friends = GetFriendList(request->userid());
        response->mutable_rescode()->set_errcode(0);
        response->mutable_rescode()->set_message("OK");
        for (const std::string& name : friends) {
            response->add_friends(name);
        }
        done->Run();
    }
};