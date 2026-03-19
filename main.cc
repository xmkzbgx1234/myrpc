#include <iostream>
#include <test.pb.h>
#include <string>

int main() {
    test::TestRequest req;
    req.set_name("xmk");
    req.set_pwd("123456");
    std::string req_str;
    if(req.SerializeToString(&req_str)) {
        std::cout << req_str << std::endl;
    }
    return 0;
}