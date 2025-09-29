#ifndef CGI_RESPONSE_HPP
#define CGI_RESPONSE_HPP

#include <string>
#include <map>
#include <vector>

/**
 * @brief CGI响应处理器
 *
 * 负责解析CGI程序的原始输出，构建符合HTTP标准的响应
 * 处理CGI headers、status、content等
 */
class CGIResponse {
public:
    /**
     * @brief 构造函数
     */
    CGIResponse();

    /**
     * @brief 析构函数
     */
    ~CGIResponse();

    /**
     * @brief 解析CGI原始输出
     *
     * CGI输出格式：
     * header1: value1\r\n
     * header2: value2\r\n
     * \r\n
     * body content
     *
     * @param rawOutput CGI程序的原始输出
     * @return true 解析成功，false 解析失败
     */
    bool parseRawOutput(const std::string& rawOutput);

    /**
     * @brief 构建完整的HTTP响应
     *
     * @return 完整的HTTP响应字符串
     */
    std::string buildHTTPResponse() const;

    /**
     * @brief 获取HTTP状态码
     *
     * @return HTTP状态码
     */
    int getStatusCode() const { return statusCode_; }

    /**
     * @brief 获取响应体
     *
     * @return 响应体内容
     */
    const std::string& getBody() const { return body_; }

    /**
     * @brief 获取特定header的值
     *
     * @param name header名称（大小写不敏感）
     * @return header值，如果不存在返回空字符串
     */
    std::string getHeader(const std::string& name) const;

    /**
     * @brief 检查是否包含特定header
     *
     * @param name header名称
     * @return true 包含，false 不包含
     */
    bool hasHeader(const std::string& name) const;

    /**
     * @brief 获取所有headers
     *
     * @return headers映射
     */
    const std::map<std::string, std::string>& getHeaders() const { return headers_; }

    /**
     * @brief 重置响应对象
     */
    void reset();

    /**
     * @brief 检查响应是否有效
     *
     * @return true 有效，false 无效
     */
    bool isValid() const { return isValid_; }

    /**
     * @brief 获取最后的错误信息
     *
     * @return 错误描述
     */
    const std::string& getLastError() const { return lastError_; }

private:
    int statusCode_;                                // HTTP状态码
    std::map<std::string, std::string> headers_;    // HTTP headers
    std::string body_;                              // 响应体
    bool isValid_;                                  // 响应是否有效
    std::string lastError_;                         // 最后的错误信息

    /**
     * @brief 解析header部分
     *
     * @param headerSection header部分的字符串
     * @return true 解析成功，false 解析失败
     */
    bool parseHeaders(const std::string& headerSection);

    /**
     * @brief 设置默认headers
     */
    void setDefaultHeaders();

    /**
     * @brief 检查header行是否有效
     *
     * @param line header行
     * @return true 有效，false 无效
     */
    bool isValidHeaderLine(const std::string& line) const;

    /**
     * @brief 解析单个header行
     *
     * @param line header行（如 "Content-Type: text/html"）
     * @param name 输出参数：header名称
     * @param value 输出参数：header值
     * @return true 解析成功，false 解析失败
     */
    bool parseHeaderLine(const std::string& line, std::string& name, std::string& value) const;

    /**
     * @brief 处理特殊的CGI headers（如Status）
     *
     * @param name header名称
     * @param value header值
     * @return true 是特殊header，false 是普通header
     */
    bool handleSpecialHeader(const std::string& name, const std::string& value);

    /**
     * @brief 规范化header名称（转为小写）
     *
     * @param name 原始名称
     * @return 规范化后的名称
     */
    std::string normalizeHeaderName(const std::string& name) const;

    /**
     * @brief 去除字符串首尾空白
     *
     * @param str 要处理的字符串
     * @return 处理后的字符串
     */
    std::string trim(const std::string& str) const;

    /**
     * @brief 设置错误信息
     *
     * @param error 错误描述
     */
    void setError(const std::string& error);

    /**
     * @brief 获取状态码对应的原因短语
     *
     * @param statusCode 状态码
     * @return 原因短语
     */
    std::string getReasonPhrase(int statusCode) const;

    /**
     * @brief 检查Content-Type是否需要默认值
     *
     * @return true 需要设置默认值，false 不需要
     */
    bool needsDefaultContentType() const;
};

#endif // CGI_RESPONSE_HPP