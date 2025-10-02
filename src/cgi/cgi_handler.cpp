#include "cgi_handler.hpp"
#include "cgi_environment.hpp"
#include "cgi_process.hpp"
#include "cgi_response.hpp"
#include <sys/stat.h>
#include <unistd.h>
#include <iostream>

CGIHandler::CGIHandler() : timeoutSeconds_(30) {
}

CGIHandler::~CGIHandler() {
}

bool CGIHandler::execute(const HttpRequest& request,
                        const LocationConfig& location,
                        const std::string& scriptPath,
                        std::string& response) {
    lastError_.clear();

    // 验证CGI执行的前置条件
    if (!validateCGIExecution(location, scriptPath)) {
        return false;
    }

    std::cout << "🔧 CGI: Executing script " << scriptPath
              << " with " << location.cgiPath << std::endl;

    try {
        // 1. 设置CGI环境变量
        CGIEnvironment environment;
        std::string scriptDir = getScriptDirectory(scriptPath);
        environment.setupEnvironment(request, scriptPath, scriptDir);

        // 2. 执行CGI进程
        CGIProcess process;
        std::string rawOutput;
        bool success = process.execute(
            location.cgiPath,       // CGI程序路径 (如 /usr/bin/python3)
            scriptPath,             // 脚本路径 (如 ./www/test.py)
            environment.getEnvArray(),
            request.getBody(),      // POST数据
            rawOutput,
            timeoutSeconds_
        );

        if (!success) {
            setError("CGI process execution failed: " + process.getLastError());
            return false;
        }

        // 3. 解析CGI输出并构建HTTP响应
        CGIResponse cgiResponse;
        if (!cgiResponse.parseRawOutput(rawOutput)) {
            setError("Failed to parse CGI output");
            return false;
        }

        response = cgiResponse.buildHTTPResponse();

        std::cout << "✅ CGI: Script executed successfully, response size: "
                  << response.size() << " bytes" << std::endl;

        return true;

    } catch (const std::exception& e) {
        setError(std::string("CGI execution exception: ") + e.what());
        return false;
    } catch (...) {
        setError("Unknown CGI execution error");
        return false;
    }
}

bool CGIHandler::isCGIRequest(const std::string& uri, const LocationConfig& location) {
    // std::cout << "🔍 Checking CGI for URI: " << uri << std::endl;
    // std::cout << "🔍 Location path: " << location.path << std::endl;
    // std::cout << "🔍 CGI Extension: '" << location.cgiExtension << "'" << std::endl;
    // std::cout << "🔍 CGI Path: '" << location.cgiPath << "'" << std::endl;

    // 检查location是否配置了CGI
    if (location.cgiExtension.empty() || location.cgiPath.empty()) {
        // std::cout << "🔍 CGI not configured for this location" << std::endl;
        return false;
    }

    // 检查URI是否匹配CGI扩展名
    std::string extension = getFileExtension(uri);
    // std::cout << "🔍 File extension: '" << extension << "'" << std::endl;
    bool matches = (extension == location.cgiExtension);
    // std::cout << "🔍 Extension matches: " << (matches ? "YES" : "NO") << std::endl;
    return matches;
}

std::string CGIHandler::getFileExtension(const std::string& filePath) {
    size_t lastDot = filePath.find_last_of('.');
    if (lastDot == std::string::npos) {
        return "";
    }
    return filePath.substr(lastDot);
}

bool CGIHandler::isCGIExecutable(const std::string& cgiPath) {
    if (cgiPath.empty()) {
        return false;
    }

    // 检查文件是否存在且可执行
    struct stat st;
    if (stat(cgiPath.c_str(), &st) != 0) {
        return false;
    }

    // 检查是否为普通文件且具有执行权限
    return S_ISREG(st.st_mode) && (st.st_mode & S_IXUSR);
}

bool CGIHandler::isScriptValid(const std::string& scriptPath) {
    if (scriptPath.empty()) {
        return false;
    }

    // 检查脚本文件是否存在且可读
    struct stat st;
    if (stat(scriptPath.c_str(), &st) != 0) {
        return false;
    }

    return S_ISREG(st.st_mode) && (st.st_mode & S_IRUSR);
}

void CGIHandler::setError(const std::string& error) {
    lastError_ = error;
    std::cerr << "❌ CGI Error: " << error << std::endl;
}

bool CGIHandler::validateCGIExecution(const LocationConfig& location, const std::string& scriptPath) {
    // 检查CGI程序路径
    if (location.cgiPath.empty()) {
        setError("CGI path not configured in location");
        return false;
    }

    if (!isCGIExecutable(location.cgiPath)) {
        setError("CGI program not executable: " + location.cgiPath);
        return false;
    }

    // 检查脚本文件
    if (!isScriptValid(scriptPath)) {
        setError("Script file not accessible: " + scriptPath);
        return false;
    }

    // 检查CGI扩展名匹配
    std::string extension = getFileExtension(scriptPath);
    if (extension != location.cgiExtension) {
        setError("Script extension mismatch: expected " + location.cgiExtension +
                ", got " + extension);
        return false;
    }

    return true;
}

std::string CGIHandler::getScriptDirectory(const std::string& scriptPath) {
    size_t lastSlash = scriptPath.find_last_of('/');
    if (lastSlash == std::string::npos) {
        return ".";  // 当前目录
    }
    return scriptPath.substr(0, lastSlash);
}