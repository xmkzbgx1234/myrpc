#include "UserService.h"
#include "FriendService.h"


int main(int argc, char** argv) {
    MyRpcApplication::Init(argc, argv);
    MyRpcProvider provider;
    provider.NotifyService(new UserService());
    provider.NotifyService(new FriendService());
    provider.Run();
    return 0;
}