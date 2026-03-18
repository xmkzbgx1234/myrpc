#pragma once

#include <iostream>
#include <string>
#include <sstream>
#include <vector>
#include <iomanip>

// Windows 特定头文件
#include <windows.h>
#include <dbghelp.h>
#include <mutex>

// 链接库指令 (如果在 IDE 中未自动链接，需手动添加 Dbghelp.lib)
#pragma comment(lib, "Dbghelp.lib")

// ==========================================
// 错误码定义
// ==========================================
enum class ErrCode {
    OK = 0,
    FAIL = 1,
    INVALID_PARAM = 2,
    NOT_FOUND = 3
};

// ==========================================
// 详细状态类 (包含堆栈信息)
// ==========================================
class DetailedStatus {
public:
    // 成功状态
    static DetailedStatus OK() {
        return DetailedStatus(true);
    }

    // 失败状态工厂：自动捕获当前堆栈
    static DetailedStatus Fail(const std::string& msg,
        const char* file, int line, const char* func) {
        DetailedStatus s(false);
        s.msg_ = msg;
        s.file_ = file;
        s.line_ = line;
        s.func_ = func;

        // 捕获堆栈
        s.stackTrace_ = captureStackTrace();

        return s;
    }

    bool isOk() const { return ok_; }

    // 生成详细的错误报告字符串
    std::string toString() const {
        if (ok_) return "Status: OK";

        std::ostringstream oss;
        oss << "========================================\n";
        oss << "ERROR DETECTED\n";
        oss << "========================================\n";
        oss << "Message: " << msg_ << "\n";
        oss << "Location: " << func_ << " ()\n";
        oss << "          File: " << file_ << "\n";
        oss << "          Line: " << line_ << "\n";
        oss << "----------------------------------------\n";
        oss << "Call Stack:\n" << stackTrace_;
        oss << "========================================\n";
        return oss.str();
    }

private:
    DetailedStatus(bool ok) : ok_(ok), line_(0) {}

    // Windows 专用的堆栈捕获与符号解析
    static std::string captureStackTrace() {
        // 1. 初始化符号处理器 (每个进程只需一次，但为了简单这里每次检查)
        // 注意：SymInitialize 不是线程安全的，实际生产环境应在全局初始化一次
        static std::mutex symMutex;
        std::lock_guard<std::mutex> lock(symMutex);

        static bool symbolsInitialized = false;
        if (!symbolsInitialized) {
            // 获取当前进程句柄
            HANDLE process = GetCurrentProcess();
            // 初始化符号，加载当前模块的 PDB 信息
            // SymSetOptions 可以设置更详细的选项，如 SYMOPT_LOAD_LINES 以获取行号
            SymSetOptions(SYMOPT_UNDNAME | SYMOPT_DEFERRED_LOADS | SYMOPT_LOAD_LINES);
            if (SymInitialize(process, NULL, TRUE)) {
                symbolsInitialized = true;
            }
            else {
                return "Failed to initialize symbol handler.\n";
            }
        }

        HANDLE process = GetCurrentProcess();

        // 2. 捕获堆栈帧 (最多 62 层，Windows API 限制)
        void* stack[62];
        USHORT frames = CaptureStackBackTrace(0, 62, stack, NULL);

        std::ostringstream oss;

        // 3. 解析每一帧
        for (USHORT i = 0; i < frames; ++i) {
            DWORD64 address = (DWORD64)(stack[i]);

            // 准备符号信息结构
            char symbolBuffer[sizeof(SYMBOL_INFO) + MAX_SYM_NAME * sizeof(TCHAR)];
            PSYMBOL_INFO pSymbol = (PSYMBOL_INFO)symbolBuffer;
            pSymbol->SizeOfStruct = sizeof(SYMBOL_INFO);
            pSymbol->MaxNameLen = MAX_SYM_NAME;

            DWORD64 symbolDisplacement = 0;
            std::string functionName = "UnknownFunction";

            // 尝试获取函数名
            if (SymFromAddr(process, address, &symbolDisplacement, pSymbol)) {
                functionName = pSymbol->Name;
            }

            // 尝试获取源文件和行号
            std::string sourceFile = "UnknownFile";
            DWORD lineNumber = 0;
            DWORD lineDisplacement = 0;
            IMAGEHLP_LINE64 lineInfo = {};
            lineInfo.SizeOfStruct = sizeof(IMAGEHLP_LINE64);

            if (SymGetLineFromAddr64(process, address, &lineDisplacement, &lineInfo)) {
                sourceFile = lineInfo.FileName;
                lineNumber = lineInfo.LineNumber;

                // 格式化输出：FuncName [File:Line]
                oss << "  #" << i << " " << functionName
                    << " [" << sourceFile << ":" << lineNumber << "]\n";
            }
            else {
                // 如果没有行号信息（例如没有 PDB 或优化过），只显示函数和地址
                oss << "  #" << i << " " << functionName
                    << " [Address: 0x" << std::hex << address << std::dec << "]\n";
            }
        }

        return oss.str();
    }

    bool ok_;
    std::string msg_;
    std::string file_;
    int line_;
    std::string func_;
    std::string stackTrace_;
};

// ==========================================
// 自动化宏
// ==========================================

// 如果表达式返回的状态不是 OK，则立即返回该状态
#define RETURN_IF_ERROR(status_expr) \
    do { \
        DetailedStatus _s = (status_expr); \
        if (!_s.isOk()) { \
            return _s; \
        } \
    } while (0)

// 创建一个带有当前位置信息的错误状态
#define MAKE_ERROR(msg) DetailedStatus::Fail(msg, __FILE__, __LINE__, __FUNCTION__)

// ==========================================
// 测试业务逻辑
// ==========================================

inline DetailedStatus deepFunction() {
    // 模拟一个深层错误
    return MAKE_ERROR("Database connection failed: Timeout");
}

inline DetailedStatus middleFunction() {
    // 调用深层函数，如果出错直接向上冒泡
    RETURN_IF_ERROR(deepFunction());
    return DetailedStatus::OK();
}

inline DetailedStatus topFunction() {
    RETURN_IF_ERROR(middleFunction());
    // 做一些其他事...
    return DetailedStatus::OK();
}
