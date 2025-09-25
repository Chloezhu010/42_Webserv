#ifndef CGI_HANDLER_HPP
#define CGI_HANDLER_HPP

#include "../http/http_request.hpp"
#include "../http/http_response.hpp"
#include "../configparser/config.hpp"
#include <string>

/**
 * @brief CGI处理器主接口类
 *
 * 负责CGI脚本的执行和管理，提供简洁的接口供WebServer使用
 * 内部协调CGIEnvironment、CGIProcess和CGIResponse等子模块
 */
class CGIHandler {
public:
    /**
     * @brief 构造函数
     */
    CGIHandler();

    /**
     * @brief 析构函数
     */
    ~CGIHandler();

    /**
     * @brief 执行CGI脚本
     *
     * @param request HTTP请求对象
     * @param location 匹配的location配置
     * @param scriptPath CGI脚本的完整路径
     * @param response 输出参数，完整的HTTP响应字符串
     * @return true 执行成功，false 执行失败
     */
    bool execute(const HttpRequest& request,
                 const LocationConfig& location,
                 const std::string& scriptPath,
                 std::string& response);

    /**
     * @brief 检查URI是否为CGI请求
     *
     * @param uri 请求URI
     * @param location location配置
     * @return true 是CGI请求，false 不是CGI请求
     */
    static bool isCGIRequest(const std::string& uri, const LocationConfig& location);

    /**
     * @brief 获取文件扩展名
     *
     * @param filePath 文件路径
     * @return 文件扩展名（包含点，如".php"）
     */
    static std::string getFileExtension(const std::string& filePath);

    /**
     * @brief 获取最后的错误信息
     *
     * @return 错误描述字符串
     */
    const std::string& getLastError() const { return lastError_; }

    /**
     * @brief 设置CGI执行超时时间
     *
     * @param seconds 超时时间（秒）
     */
    void setTimeout(int seconds) { timeoutSeconds_ = seconds; }

    /**
     * @brief 获取当前超时设置
     *
     * @return 超时时间（秒）
     */
    int getTimeout() const { return timeoutSeconds_; }

    /**
     * @brief 检查CGI程序是否存在且可执行
     *
     * @param cgiPath CGI程序路径
     * @return true 可执行，false 不可执行
     */
    static bool isCGIExecutable(const std::string& cgiPath);

    /**
     * @brief 验证脚本文件是否存在
     *
     * @param scriptPath 脚本文件路径
     * @return true 存在，false 不存在
     */
    static bool isScriptValid(const std::string& scriptPath);

private:
    std::string lastError_;     // 最后的错误信息
    int timeoutSeconds_;        // CGI执行超时时间（默认30秒）

    /**
     * @brief 设置错误信息
     *
     * @param error 错误描述
     */
    void setError(const std::string& error);

    /**
     * @brief 验证CGI执行的前置条件
     *
     * @param location location配置
     * @param scriptPath 脚本路径
     * @return true 验证通过，false 验证失败
     */
    bool validateCGIExecution(const LocationConfig& location, const std::string& scriptPath);

    /**
     * @brief 获取脚本所在目录
     *
     * @param scriptPath 脚本完整路径
     * @return 目录路径
     */
    static std::string getScriptDirectory(const std::string& scriptPath);

    // 禁止拷贝构造和赋值
    CGIHandler(const CGIHandler&);
    CGIHandler& operator=(const CGIHandler&);
};

#endif // CGI_HANDLER_HPP