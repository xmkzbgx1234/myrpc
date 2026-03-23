#pragma once

#include <iostream>
#include <string>
#include <sstream>
#include <vector>
#include <iomanip>
#include <cstdlib>
#include <memory>
#include "log.h"

#ifdef _WIN32
#include <windows.h>
#include <dbghelp.h>
#include <mutex>
#pragma comment(lib, "Dbghelp.lib")
#else
#include <execinfo.h>
#include <cxxabi.h>
#endif

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

        LOG(LogLevel::ERROR) << "========================================\n";
        LOG(LogLevel::ERROR) << "ERROR DETECTED\n";
        LOG(LogLevel::ERROR) << "Message: " << msg_ << "\n";
        LOG(LogLevel::ERROR) << "----------------------------------------\n";
        LOG(LogLevel::ERROR) << "Call Stack:\n" << stackTrace_;
        LOG(LogLevel::ERROR) << "Location: " << func_ << " ()\n";
        LOG(LogLevel::ERROR) << "          File: " << file_ << "\n";
        LOG(LogLevel::ERROR) << "          Line: " << line_ << "\n";
        LOG(LogLevel::ERROR) << "----------------------------------------\n";
        LOG(LogLevel::ERROR) << "Call Stack:\n" << stackTrace_;
        LOG(LogLevel::ERROR) << "========================================\n";
    }

private:
    DetailedStatus(bool ok) : ok_(ok), line_(0) {}

    static std::string captureStackTrace() {
#ifdef _WIN32
        // 1. 初始化符号处理器 (每个进程只需一次，但为了简单这里每次检查)
        // 注意：SymInitialize 不是线程安全的，实际生产环境应在全局初始化一次
        static std::mutex symMutex;
        std::lock_guard<std::mutex> lock(symMutex);

        static bool symbolsInitialized = false;
        if (!symbolsInitialized) {
            HANDLE process = GetCurrentProcess();
            SymSetOptions(SYMOPT_UNDNAME | SYMOPT_DEFERRED_LOADS | SYMOPT_LOAD_LINES);
            if (SymInitialize(process, NULL, TRUE)) {
                symbolsInitialized = true;
            }
            else {
                return "Failed to initialize symbol handler.\n";
            }
        }

        HANDLE process = GetCurrentProcess();

        void* stack[62];
        USHORT frames = CaptureStackBackTrace(0, 62, stack, NULL);

        std::ostringstream oss;

        for (USHORT i = 0; i < frames; ++i) {
            DWORD64 address = (DWORD64)(stack[i]);

            char symbolBuffer[sizeof(SYMBOL_INFO) + MAX_SYM_NAME * sizeof(TCHAR)];
            PSYMBOL_INFO pSymbol = (PSYMBOL_INFO)symbolBuffer;
            pSymbol->SizeOfStruct = sizeof(SYMBOL_INFO);
            pSymbol->MaxNameLen = MAX_SYM_NAME;

            DWORD64 symbolDisplacement = 0;
            std::string functionName = "UnknownFunction";

            if (SymFromAddr(process, address, &symbolDisplacement, pSymbol)) {
                functionName = pSymbol->Name;
            }

            std::string sourceFile = "UnknownFile";
            DWORD lineNumber = 0;
            DWORD lineDisplacement = 0;
            IMAGEHLP_LINE64 lineInfo = {};
            lineInfo.SizeOfStruct = sizeof(IMAGEHLP_LINE64);

            if (SymGetLineFromAddr64(process, address, &lineDisplacement, &lineInfo)) {
                sourceFile = lineInfo.FileName;
                lineNumber = lineInfo.LineNumber;
                oss << "  #" << i << " " << functionName
                    << " [" << sourceFile << ":" << lineNumber << "]\n";
            }
            else {
                oss << "  #" << i << " " << functionName
                    << " [Address: 0x" << std::hex << address << std::dec << "]\n";
            }
        }

        return oss.str();
#else
        constexpr int kMaxFrames = 64;
        void* frames[kMaxFrames];
        int frameCount = backtrace(frames, kMaxFrames);
        if (frameCount <= 0) {
            LOG(LogLevel::ERROR) << "No stack trace available.\n";
            return "No stack trace available.\n";
        }

        char** symbols = backtrace_symbols(frames, frameCount);
        if (symbols == nullptr) {
            LOG(LogLevel::ERROR) << "Failed to resolve stack symbols.\n";
            return "Failed to resolve stack symbols.\n";
        }

        std::unique_ptr<char*, decltype(&std::free)> holder(symbols, &std::free);
        std::ostringstream oss;

        for (int i = 0; i < frameCount; ++i) {
            std::string symbolLine = symbols[i];
            std::string functionName = symbolLine;

            // Common Linux pattern:
            // /path/bin(function+0x15c) [0x55...]
            std::size_t left = symbolLine.find('(');
            std::size_t plus = symbolLine.find('+', left);
            if (left != std::string::npos && plus != std::string::npos && plus > left + 1) {
                std::string mangled = symbolLine.substr(left + 1, plus - left - 1);
                int status = 0;
                std::unique_ptr<char, decltype(&std::free)> demangled(
                    abi::__cxa_demangle(mangled.c_str(), nullptr, nullptr, &status), &std::free);
                if (status == 0 && demangled) {
                    functionName = demangled.get();
                } else {
                    functionName = mangled;
                }
            }

            oss << "  #" << i << " " << functionName << "\n";
        }

        return oss.str();
#endif
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
