#include "cgi_response.hpp"
#include <sstream>
#include <algorithm>
#include <iostream>

CGIResponse::CGIResponse() : statusCode_(200), isValid_(false) {
}

CGIResponse::~CGIResponse() {
}

bool CGIResponse::parseRawOutput(const std::string& rawOutput) {
    lastError_.clear();
    reset();

    if (rawOutput.empty()) {
        setError("Empty CGI output");
        return false;
    }

    // 查找header和body的分界线
    size_t headerEnd = rawOutput.find("\r\n\r\n");
    if (headerEnd == std::string::npos) {
        headerEnd = rawOutput.find("\n\n");
        if (headerEnd == std::string::npos) {
            // 没有找到分界线，可能全是body
            body_ = rawOutput;
            setDefaultHeaders();
            isValid_ = true;
            return true;
        }
    }

    // 分离header和body
    std::string headerSection = rawOutput.substr(0, headerEnd);
    body_ = rawOutput.substr(headerEnd + 4); // +4 跳过 \r\n\r\n

    // 解析headers
    if (!parseHeaders(headerSection)) {
        return false;
    }

    // 设置默认headers
    setDefaultHeaders();

    isValid_ = true;
    return true;
}

bool CGIResponse::parseHeaders(const std::string& headerSection) {
    std::istringstream iss(headerSection);
    std::string line;

    while (std::getline(iss, line)) {
        // 移除回车符
        if (!line.empty() && line[line.length() - 1] == '\r') {
            line.erase(line.length() - 1);
        }

        if (line.empty()) {
            continue;
        }

        if (!isValidHeaderLine(line)) {
            setError("Invalid header line: " + line);
            return false;
        }

        std::string name, value;
        if (!parseHeaderLine(line, name, value)) {
            continue;
        }

        // 处理特殊headers或添加到普通headers
        if (!handleSpecialHeader(name, value)) {
            headers_[normalizeHeaderName(name)] = value;
        }
    }

    return true;
}

bool CGIResponse::parseHeaderLine(const std::string& line, std::string& name, std::string& value) const {
    size_t colonPos = line.find(':');
    if (colonPos == std::string::npos) {
        return false;
    }

    name = trim(line.substr(0, colonPos));
    value = trim(line.substr(colonPos + 1));

    return !name.empty();
}

bool CGIResponse::handleSpecialHeader(const std::string& name, const std::string& value) {
    std::string lowerName = normalizeHeaderName(name);

    if (lowerName == "status") {
        // 解析状态码
        std::istringstream iss(value);
        iss >> statusCode_;
        return true;
    }

    return false; // 不是特殊header
}

std::string CGIResponse::buildHTTPResponse() const {
    std::ostringstream response;

    // 状态行
    response << "HTTP/1.1 " << statusCode_ << " " << getReasonPhrase(statusCode_) << "\r\n";

    // Headers
    for (std::map<std::string, std::string>::const_iterator it = headers_.begin();
         it != headers_.end(); ++it) {
        response << it->first << ": " << it->second << "\r\n";
    }

    response << "\r\n";

    // Body
    response << body_;

    return response.str();
}

void CGIResponse::setDefaultHeaders() {
    // 设置Content-Length
    if (headers_.find("content-length") == headers_.end()) {
        std::ostringstream oss;
        oss << body_.length();
        headers_["content-length"] = oss.str();
    }

    // 设置Content-Type
    if (needsDefaultContentType()) {
        headers_["content-type"] = "text/html";
    }

    // 设置Connection
    if (headers_.find("connection") == headers_.end()) {
        headers_["connection"] = "close";
    }
}

bool CGIResponse::needsDefaultContentType() const {
    return headers_.find("content-type") == headers_.end();
}

std::string CGIResponse::getHeader(const std::string& name) const {
    std::string normalizedName = normalizeHeaderName(name);
    std::map<std::string, std::string>::const_iterator it = headers_.find(normalizedName);
    return (it != headers_.end()) ? it->second : "";
}

bool CGIResponse::hasHeader(const std::string& name) const {
    std::string normalizedName = normalizeHeaderName(name);
    return headers_.find(normalizedName) != headers_.end();
}

std::string CGIResponse::normalizeHeaderName(const std::string& name) const {
    std::string result = name;
    std::transform(result.begin(), result.end(), result.begin(), ::tolower);
    return result;
}

std::string CGIResponse::trim(const std::string& str) const {
    size_t start = str.find_first_not_of(" \t");
    if (start == std::string::npos) {
        return "";
    }
    size_t end = str.find_last_not_of(" \t");
    return str.substr(start, end - start + 1);
}

bool CGIResponse::isValidHeaderLine(const std::string& line) const {
    return line.find(':') != std::string::npos;
}

std::string CGIResponse::getReasonPhrase(int statusCode) const {
    switch (statusCode) {
        case 200: return "OK";
        case 201: return "Created";
        case 400: return "Bad Request";
        case 404: return "Not Found";
        case 500: return "Internal Server Error";
        case 502: return "Bad Gateway";
        default: return "Unknown";
    }
}

void CGIResponse::reset() {
    statusCode_ = 200;
    headers_.clear();
    body_.clear();
    isValid_ = false;
    lastError_.clear();
}

void CGIResponse::setError(const std::string& error) {
    lastError_ = error;
    isValid_ = false;
}