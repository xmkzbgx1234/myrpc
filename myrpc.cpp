// myrpc.cpp: 定义应用程序的入口点。
//

#include "myrpc.h"
#include "common/log.h"
#include "common/config.h"
#include "common/status.h"

using namespace std;


int main() {
    // 设置控制台输出编码为 UTF-8 (可选，防止中文乱码)
    SetConsoleOutputCP(CP_UTF8);

    std::cout << "Starting Windows Error Trace Demo...\n\n";

    DetailedStatus status = topFunction();

    if (!status.isOk()) {
        // 打印包含完整堆栈的错误信息
        std::cerr << status.toString() << std::endl;
    }
    else {
        std::cout << "Success!" << std::endl;
    }

    // 防止控制台立即关闭
    // system("pause"); 
    return 0;
}