# pragma once

#include <iostream>
#include <fstream>
#include <sstream>
#include <string>
#include <map>
#include <memory>
#include <stdexcept>

// 异常类，用于报告配置错误
class ConfigException : public std::runtime_error {
public:
    ConfigException(const std::string& msg) : std::runtime_error(msg) {}
};

class Config {
public:
    bool load(const std::string& filename) {
        std::ifstream file(filename);
        if (!file.is_open()) throw ConfigException("Cannot open file: " + filename);

        std::string line, currentSection = "default";
        while (std::getline(file, line)) {
            trim(line);
            if (line.empty() || line[0] == '#') continue;

            // 检测 Section: [name]
            if (line.front() == '[' && line.back() == ']') {
                currentSection = line.substr(1, line.size() - 2);
                trim(currentSection);
                continue;
            }

            // 检测 Key=Value
            size_t pos = line.find('=');
            if (pos != std::string::npos) {
                std::string key = line.substr(0, pos);
                std::string value = line.substr(pos + 1);
                trim(key);
                trim(value);
                data_[currentSection][key] = value;
            }
        }
        return true;
    }

    // 获取值的通用模板接口
    template<typename T>
    T get(const std::string& section, const std::string& key, const T& defaultVal) const;

    // 检查是否存在
    bool has(const std::string& section, const std::string& key) const {
        auto secIt = data_.find(section);
        if (secIt == data_.end()) return false;
        return secIt->second.find(key) != secIt->second.end();
    }

private:
    // Section -> (Key -> Value)
    std::map<std::string, std::map<std::string, std::string>> data_;

    void trim(std::string& s) {
        if (s.empty()) return;
        s.erase(0, s.find_first_not_of(" \t\r\n"));
        s.erase(s.find_last_not_of(" \t\r\n") + 1);
    }
};

// 模板特化实现类型转换
template<>
inline
std::string Config::get<std::string>(const std::string& section, const std::string& key, const std::string& defaultVal) const {
    auto secIt = data_.find(section);
    if (secIt == data_.end()) return defaultVal;
    auto keyIt = secIt->second.find(key);
    return (keyIt != secIt->second.end()) ? keyIt->second : defaultVal;
}

template<>
inline
int Config::get<int>(const std::string& section, const std::string& key, const int& defaultVal) const {
    std::string val = get<std::string>(section, key, "");
    if (val.empty()) return defaultVal;
    try {
        return std::stoi(val);
    }
    catch (...) {
        throw ConfigException("Invalid integer format for [" + section + "]" + key);
    }
}

template<>
inline
bool Config::get<bool>(const std::string& section, const std::string& key, const bool& defaultVal) const {
    std::string val = get<std::string>(section, key, "");
    if (val.empty()) return defaultVal;
    return (val == "true" || val == "1" || val == "yes");
}

// 业务配置结构体
struct ServerConfig {
    std::string host;
    int port;

    // 从 Config 对象加载自身
    void loadFrom(const Config& cfg) {
        host = cfg.get<std::string>("rpcserver", "rpcserverip", "localhost");
        port = cfg.get<int>("rpcserver", "rpcserverport", 8080);

        // 可选：强制校验关键配置是否存在
        if (!cfg.has("rpcserver", "rpcserverip")) {
            throw ConfigException("Missing critical config: rpcserver.rpcserverip");
        }
        if(!cfg.has("rpcserver", "rpcserverport")) {
            throw ConfigException("Missing critical config: rpcserver.rpcserverport");
        }
    }
};
