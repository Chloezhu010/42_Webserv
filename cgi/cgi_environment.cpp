#include "cgi_environment.hpp"
#include <sstream>
#include <iostream>

CGIEnvironment::CGIEnvironment() {
}

CGIEnvironment::~CGIEnvironment() {
    clear();
}

void CGIEnvironment::setupEnvironment(const HttpRequest& request,
                                    const std::string& scriptPath,
                                    const std::string& serverRoot) {
    (void)serverRoot; // 暂时未使用，避免编译警告
    clear();

    // 添加标准CGI环境变量
    addStandardVars(request, scriptPath);
    addServerVars();
    addRequestVars(request);

    // 构建环境变量数组
    buildEnvArray();
}

void CGIEnvironment::addStandardVars(const HttpRequest& request, const std::string& scriptPath) {
    // 基本CGI变量
    addVar("REQUEST_METHOD", request.getMethodStr());
    addVar("SCRIPT_NAME", scriptPath);
    addVar("PATH_INFO", scriptPath);
    addVar("QUERY_STRING", request.getQueryString());
    addVar("CONTENT_LENGTH", toString(request.getBody().length()));
    // TODO: 实现getContentType()后再添加
    // addVar("CONTENT_TYPE", request.getContentType());
    addVar("CONTENT_TYPE", "");
}

void CGIEnvironment::addServerVars() {
    // 服务器变量
    addVar("SERVER_SOFTWARE", "webserv/1.0");
    addVar("SERVER_NAME", "localhost");
    addVar("GATEWAY_INTERFACE", "CGI/1.1");
    addVar("SERVER_PROTOCOL", "HTTP/1.1");
    addVar("SERVER_PORT", "8080");
}

void CGIEnvironment::addRequestVars(const HttpRequest& request) {
    // 请求相关变量
    addVar("HTTP_HOST", request.getHost());
    // TODO: 实现getUserAgent()后再添加
    // addVar("HTTP_USER_AGENT", request.getUserAgent());
}

void CGIEnvironment::addVar(const std::string& name, const std::string& value) {
    envMap_[name] = value;
    envStrings_.push_back(name + "=" + value);
}

void CGIEnvironment::addCustomVar(const std::string& name, const std::string& value) {
    addVar(name, value);
    buildEnvArray(); // 重建数组
}

void CGIEnvironment::buildEnvArray() {
    envArray_.clear();
    for (size_t i = 0; i < envStrings_.size(); ++i) {
        envArray_.push_back(const_cast<char*>(envStrings_[i].c_str()));
    }
    envArray_.push_back(NULL); // NULL终止
}

char** CGIEnvironment::getEnvArray() {
    if (envArray_.empty()) {
        buildEnvArray();
    }
    return &envArray_[0];
}

void CGIEnvironment::clear() {
    envStrings_.clear();
    envArray_.clear();
    envMap_.clear();
}

void CGIEnvironment::printEnvironment() const {
    std::cout << "=== CGI Environment Variables ===" << std::endl;
    for (size_t i = 0; i < envStrings_.size(); ++i) {
        std::cout << envStrings_[i] << std::endl;
    }
    std::cout << "=================================" << std::endl;
}

template<typename T>
std::string CGIEnvironment::toString(T value) {
    std::ostringstream oss;
    oss << value;
    return oss.str();
}