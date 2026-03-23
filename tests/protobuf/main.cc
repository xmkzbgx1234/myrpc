#include <iostream>
#include <string>

#include "test.pb.h"

int main() {
    GOOGLE_PROTOBUF_VERIFY_VERSION;

    test::TestRequest req;
    req.set_name("alice");
    req.set_pwd("123456");

    std::string serialized;
    if (!req.SerializeToString(&serialized)) {
        std::cerr << "SerializeToString failed" << std::endl;
        return 1;
    }

    test::TestRequest parsed;
    if (!parsed.ParseFromString(serialized)) {
        std::cerr << "ParseFromString failed" << std::endl;
        return 2;
    }

    std::cout << "Serialize/Parse OK: "
              << "name=" << parsed.name()
              << ", pwd=" << parsed.pwd() << std::endl;

    google::protobuf::ShutdownProtobufLibrary();
    return 0;
}
