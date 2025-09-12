#include "http_response.hpp"
#include <fstream>
#include <iomanip>

// ============================================================================
// 构造函数和析构函数
// ============================================================================

HttpResponse::HttpResponse() : status_code_(200), content_type_("text/html; charset=UTF-8")
{
}

HttpResponse::HttpResponse(int status_code) : status_code_(status_code), content_type_("text/html; charset=UTF-8")
{
}

HttpResponse::~HttpResponse()
{
}

// ============================================================================
// 状态行相关方法
// ============================================================================

void HttpResponse::setStatusCode(int code)
{
    status_code_ = code;
}

/* 将ValidationResult枚举转换为HTTP状态码 */
void HttpResponse::resultToStatusCode(ValidationResult result)
{
    switch (result)
    {
        // 2xx 成功响应
        case VALID_REQUEST: status_code_ = 200; break;
        case CREATED: status_code_ = 201; break;
        case NO_CONTENT: status_code_ = 204; break;
        
        // 3xx 重定向
        case MOVED_PERMANENTLY: status_code_ = 301; break;
        case FOUND: status_code_ = 302; break;
        
        // 4xx 客户端错误
        case BAD_REQUEST:
        case INVALID_REQUEST_LINE:
        case INVALID_HTTP_VERSION:
        case INVALID_URI:
        case INVALID_HEADER:
        case INVALID_CONTENT_LENGTH:
        case CONFLICTING_HEADER:
        case METHOD_BODY_MISMATCH:
        case MISSING_HOST_HEADER:
            status_code_ = 400; break;
        case UNAUTORIZED: status_code_ = 401; break;
        case FORBIDDEN: status_code_ = 403; break;
        case NOT_FOUND: status_code_ = 404; break;
        case INVALID_METHOD: status_code_ = 405; break;
        case REQUEST_TIMEOUT: status_code_ = 408; break;
        case CONFLICT: status_code_ = 409; break;
        case LENGTH_REQUIRED: status_code_ = 411; break;
        case PAYLOAD_TOO_LARGE: status_code_ = 413; break;
        case URI_TOO_LONG: status_code_ = 414; break;
        case UNSUPPORTED_MEDIA_TYPE: status_code_ = 415; break;
        case HEADER_TOO_LARGE: status_code_ = 431; break;
        
        // 5xx 服务器错误
        case INTERNAL_SERVER_ERROR: status_code_ = 500; break;
        case NOT_IMPLEMENTED: status_code_ = 501; break;
        case BAD_GATEWAY: status_code_ = 502; break;
        case SERVICE_UNAVAILABLE: status_code_ = 503; break;
        case GATEWAY_TIMEOUT: status_code_ = 504; break;
        
        default: status_code_ = 500; break;
    }
}

/* 获取状态码对应的原因短语 */
std::string HttpResponse::getReasonPhrase() const
{
    switch (status_code_)
    {
        // 2xx 成功响应
        case 200: return "OK";
        case 201: return "Created";
        case 204: return "No Content";
        
        // 3xx 重定向  
        case 301: return "Moved Permanently";
        case 302: return "Found";
        
        // 4xx 客户端错误
        case 400: return "Bad Request";
        case 401: return "Unauthorized";
        case 403: return "Forbidden";
        case 404: return "Not Found";
        case 405: return "Method Not Allowed";
        case 408: return "Request Timeout";
        case 409: return "Conflict";
        case 411: return "Length Required";
        case 413: return "Payload Too Large";
        case 414: return "URI Too Long";
        case 415: return "Unsupported Media Type";
        case 431: return "Request Header Fields Too Large";
        
        // 5xx 服务器错误
        case 500: return "Internal Server Error";
        case 501: return "Not Implemented";
        case 502: return "Bad Gateway";
        case 503: return "Service Unavailable";
        case 504: return "Gateway Timeout";
        
        default: return "Unknown Status";
    }
}

/* 构建HTTP状态行 */
std::string HttpResponse::buildStatusLine() const
{
    std::ostringstream oss;
    oss << "HTTP/1.1 " << status_code_ << " " << getReasonPhrase() << "\r\n";
    return oss.str();
}

// ============================================================================
// 响应头相关方法
// ============================================================================

void HttpResponse::setHeader(const std::string& name, const std::string& value)
{
    headers_[name] = value;
}

void HttpResponse::removeHeader(const std::string& name)
{
    headers_.erase(name);
}

std::string HttpResponse::getHeader(const std::string& name) const
{
    std::map<std::string, std::string>::const_iterator it = headers_.find(name);
    if (it != headers_.end())
        return it->second;
    return "";
}

/* 获取HTTP格式的当前GMT时间 (RFC 7231) */
std::string HttpResponse::getCurrentDateGMT() const
{
    time_t now = time(0);
    struct tm* gmt = gmtime(&now);
    
    char buffer[100];
    strftime(buffer, sizeof(buffer), "%a, %d %b %Y %H:%M:%S GMT", gmt);
    return std::string(buffer);
}

/* 根据文件扩展名确定内容类型 */
std::string HttpResponse::getContentType(const std::string& file_path) const
{
    if (file_path.empty())
        return "text/html; charset=UTF-8";
        
    // 查找文件扩展名
    size_t dot_pos = file_path.find_last_of('.');
    if (dot_pos == std::string::npos)
        return "text/plain; charset=UTF-8";
        
    std::string ext = file_path.substr(dot_pos + 1);
    std::transform(ext.begin(), ext.end(), ext.begin(), ::tolower);
    
    // 将扩展名映射到MIME类型
    if (ext == "html" || ext == "htm")
        return "text/html; charset=UTF-8";
    else if (ext == "css")
        return "text/css; charset=UTF-8";
    else if (ext == "js")
        return "application/javascript; charset=UTF-8";
    else if (ext == "json")
        return "application/json; charset=UTF-8";
    else if (ext == "xml")
        return "application/xml; charset=UTF-8";
    else if (ext == "txt")
        return "text/plain; charset=UTF-8";
    else if (ext == "jpg" || ext == "jpeg")
        return "image/jpeg";
    else if (ext == "png")
        return "image/png";
    else if (ext == "gif")
        return "image/gif";
    else if (ext == "ico")
        return "image/x-icon";
    else if (ext == "pdf")
        return "application/pdf";
    else if (ext == "zip")
        return "application/zip";
    else
        return "application/octet-stream";
}

/* 设置标准HTTP响应头 */
void HttpResponse::setStandardHeaders(const HttpRequest& request)
{
    // 设置必需的响应头
    setHeader("Server", "42_webserv/1.0");
    setHeader("Date", getCurrentDateGMT());
    
    // 设置内容长度
    std::ostringstream content_length_stream;
    content_length_stream << body_.length();
    setHeader("Content-Length", content_length_stream.str());
    
    // 根据请求设置连接头
    bool keep_alive = request.getConnection();
    if (keep_alive && status_code_ < 400) // 错误时不保持连接
        setHeader("Connection", "keep-alive");
    else
        setHeader("Connection", "close");
    
    // 设置内容类型
    if (!content_type_.empty())
        setHeader("Content-Type", content_type_);
}

/* 设置内容相关的响应头 */
void HttpResponse::setContentHeaders(const std::string& content, const std::string& file_path)
{
    content_type_ = getContentType(file_path);
    setHeader("Content-Type", content_type_);
    
    std::ostringstream oss;
    oss << content.length();
    setHeader("Content-Length", oss.str());
    
    // 为静态文件添加缓存头
    if (!file_path.empty() && status_code_ == 200)
    {
        setHeader("Cache-Control", "public, max-age=3600"); // 缓存1小时
        setHeader("ETag", "\"" + file_path + "\""); // 基于文件路径的简单ETag
    }
}

/* 构建响应头部分 */
std::string HttpResponse::buildHeaders() const
{
    std::string header_section;
    
    // 定义响应头的顺序以保持一致性
    std::string header_order[] = {
        "Server", "Date", "Content-Type", "Content-Length", 
        "Connection", "Cache-Control", "ETag"
    };
    
    // 按顺序添加响应头
    for (size_t i = 0; i < sizeof(header_order) / sizeof(header_order[0]); ++i)
    {
        std::map<std::string, std::string>::const_iterator it = headers_.find(header_order[i]);
        if (it != headers_.end())
        {
            header_section += it->first + ": " + it->second + "\r\n";
        }
    }
    
    // 添加其他不在有序列表中的响应头
    for (std::map<std::string, std::string>::const_iterator it = headers_.begin(); 
         it != headers_.end(); ++it)
    {
        bool found_in_order = false;
        for (size_t i = 0; i < sizeof(header_order) / sizeof(header_order[0]); ++i)
        {
            if (it->first == header_order[i])
            {
                found_in_order = true;
                break;
            }
        }
        if (!found_in_order)
        {
            header_section += it->first + ": " + it->second + "\r\n";
        }
    }
    
    return header_section;
}

// ============================================================================
// 响应体相关方法
// ============================================================================

void HttpResponse::setBody(const std::string& body)
{
    body_ = body;
    // 更新Content-Length响应头
    std::ostringstream oss;
    oss << body_.length();
    setHeader("Content-Length", oss.str());
}

void HttpResponse::setBodyFromFile(const std::string& file_path)
{
    std::ifstream file(file_path.c_str());
    if (!file.is_open())
    {
        setStatusCode(404);
        setBody(generateErrorPage(404, "文件未找到"));
        content_type_ = "text/html; charset=UTF-8";
        return;
    }
    
    std::ostringstream buffer;
    buffer << file.rdbuf();
    file.close();
    
    std::string content = buffer.str();
    setBody(content);
    setContentHeaders(content, file_path);
}

void HttpResponse::appendBody(const std::string& content)
{
    body_ += content;
    // 更新Content-Length响应头
    std::ostringstream oss;
    oss << body_.length();
    setHeader("Content-Length", oss.str());
}

void HttpResponse::clearBody()
{
    body_.clear();
    setHeader("Content-Length", "0");
}

/* 生成错误页面HTML */
std::string HttpResponse::generateErrorPage(int status_code, const std::string& reason) const
{
    std::ostringstream html;
    std::string status_text;
    std::string error_message;
    
    switch (status_code)
    {
        case 400: status_text = "错误请求"; error_message = "请求格式不正确"; break;
        case 403: status_text = "禁止访问"; error_message = "您没有权限访问此资源"; break;
        case 404: status_text = "页面未找到"; error_message = "抱歉，您请求的页面不存在"; break;
        case 405: status_text = "方法不允许"; error_message = "此请求方法不被允许"; break;
        case 500: status_text = "内部服务器错误"; error_message = "服务器遇到内部错误"; break;
        default: status_text = "错误"; error_message = "发生了未知错误"; break;
    }
    
    html << "<!DOCTYPE html>\n"
         << "<html lang=\"zh-CN\">\n"
         << "<head>\n"
         << "    <meta charset=\"UTF-8\">\n"
         << "    <meta name=\"viewport\" content=\"width=device-width, initial-scale=1.0\">\n"
         << "    <title>" << status_code << " - " << status_text << "</title>\n"
         << "    <style>\n"
         << "        body { font-family: 'Microsoft YaHei', Arial, sans-serif; text-align: center; padding: 50px; background-color: #f8f9fa; }\n"
         << "        .error-container { max-width: 500px; margin: 0 auto; background: white; padding: 40px; border-radius: 10px; box-shadow: 0 2px 10px rgba(0,0,0,0.1); }\n"
         << "        h1 { color: #dc3545; font-size: 4em; margin: 0; }\n"
         << "        h2 { color: #333; margin: 20px 0; }\n"
         << "        p { color: #666; margin: 20px 0; }\n"
         << "        a { color: #007bff; text-decoration: none; }\n"
         << "        a:hover { text-decoration: underline; }\n"
         << "        .error-details { background: #f8f9fa; padding: 15px; border-radius: 5px; margin: 20px 0; }\n"
         << "    </style>\n"
         << "</head>\n"
         << "<body>\n"
         << "    <div class=\"error-container\">\n"
         << "        <h1>" << status_code << "</h1>\n"
         << "        <h2>" << status_text << "</h2>\n"
         << "        <p>" << (reason.empty() ? error_message : reason) << "</p>\n";
    
    // 添加一些可能的原因说明
    if (status_code == 404)
    {
        html << "        <div class=\"error-details\">\n"
             << "            <p>可能的原因：</p>\n"
             << "            <ul style=\"text-align: left; color: #666;\">\n"
             << "                <li>URL地址输入错误</li>\n"
             << "                <li>页面已被删除或移动</li>\n"
             << "                <li>服务器配置问题</li>\n"
             << "            </ul>\n"
             << "        </div>\n";
    }
    
    html << "        <p><a href=\"/\">返回首页</a></p>\n"
         << "        <hr>\n"
         << "        <small>42_webserv/1.0 服务器</small>\n"
         << "    </div>\n"
         << "</body>\n"
         << "</html>";
    
    return html.str();
}

// ============================================================================
// 响应构建方法
// ============================================================================

/* 构建完整的HTTP响应 */
std::string HttpResponse::buildFullResponse(const HttpRequest& request)
{
    // ⚠️ 关键修改：只有在状态码还是默认值时才根据验证结果设置
    if (status_code_ == 200 && request.getValidationStatus() != VALID_REQUEST) {
        resultToStatusCode(request.getValidationStatus());
    }
    
    // 如果是错误状态且没有响应体，生成错误页面
    if (isErrorStatus() && body_.empty()) {
        setBody(generateErrorPage(status_code_, ""));
        content_type_ = "text/html; charset=UTF-8";
    }
    
    // 设置标准响应头
    setStandardHeaders(request);
    
    // 构建响应组件
    std::string status_line = buildStatusLine();
    std::string headers = buildHeaders();
    
    return status_line + headers + "\r\n" + body_;
}

/* 构建错误响应 */
std::string HttpResponse::buildErrorResponse(int status_code, const std::string& message)
{
    setStatusCode(status_code);
    setBody(generateErrorPage(status_code, message));
    content_type_ = "text/html; charset=UTF-8";
    
    // 在没有请求上下文的情况下构建基本响应头
    setHeader("Server", "42_webserv/1.0");
    setHeader("Date", getCurrentDateGMT());
    setHeader("Connection", "close");
    setContentHeaders(body_, "");
    
    std::string status_line = buildStatusLine();
    std::string headers = buildHeaders();
    
    return status_line + headers + "\r\n" + body_;
}

/* 构建文件响应 */
std::string HttpResponse::buildFileResponse(const std::string& file_path)
{
    setBodyFromFile(file_path);
    
    if (status_code_ != 404) // 文件存在
        setStatusCode(200);
    
    // 在没有请求上下文的情况下构建基本响应头
    setHeader("Server", "42_webserv/1.0");
    setHeader("Date", getCurrentDateGMT());
    setHeader("Connection", "close");
    
    std::string status_line = buildStatusLine();
    std::string headers = buildHeaders();
    
    return status_line + headers + "\r\n" + body_;
}

// ============================================================================
// Getter方法和工具方法
// ============================================================================

int HttpResponse::getStatusCode() const
{
    return status_code_;
}

const std::string& HttpResponse::getBody() const
{
    return body_;
}

size_t HttpResponse::getContentLength() const
{
    return body_.length();
}

void HttpResponse::reset()
{
    status_code_ = 200;
    status_line_.clear();
    headers_.clear();
    body_.clear();
    content_type_ = "text/html; charset=UTF-8";
}

bool HttpResponse::isErrorStatus() const
{
    return status_code_ >= 400;
}

bool HttpResponse::isSuccessStatus() const
{
    return status_code_ >= 200 && status_code_ < 300;
}