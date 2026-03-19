#include <iostream>
#include "../user.pb.h"
#include <string>
class UserService : public fixbug::UserServiceRpc{
public:
    bool Login(std::string name, std::string pwd) {
        std::cout << "Login: name=" << name << ", pwd=" << pwd << std::endl;
        return true;
    }
    void Login(::google::protobuf::RpcController* controller,
                    const ::fixbug::LoginRequest* request,
                    ::fixbug::LoginResponse*  response,
                    ::google::protobuf::Closure* done){
        std::string name = request->name();
        std::string pwd = request->pwd();
        bool success = Login(name, pwd);
        response->mutable_rescode()->set_errcode(success ? 0 : 1);
        response->mutable_rescode()->set_message(success ? "OK" : "FAIL");
        response->set_success(success);
        done->Run();
    }
};

int main(){
    return 0;
}