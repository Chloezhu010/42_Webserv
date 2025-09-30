#ifndef CGI_ENVIRONMENT_HPP
#define CGI_ENVIRONMENT_HPP

#include "../http/http_request.hpp"
#include <vector>
#include <string>
#include <map>

/**
 * @brief CGI环境变量管理器
 *
 * 负责设置和管理CGI脚本执行时需要的环境变量
 * 符合CGI/1.1标准规范
 */
class CGIEnvironment {
public:
    /**
     * @brief 构造函数
     */
    CGIEnvironment();

    /**
     * @brief 析构函数
     */
    ~CGIEnvironment();

    /**
     * @brief 设置CGI环境变量
     *
     * @param request HTTP请求对象
     * @param scriptPath CGI脚本的完整路径
     * @param serverRoot 服务器根目录（用于chdir）
     */
    void setupEnvironment(const HttpRequest& request,
                         const std::string& scriptPath,
                         const std::string& serverRoot = ".");

    /**
     * @brief 添加自定义环境变量
     *
     * @param name 变量名
     * @param value 变量值
     */
    void addCustomVar(const std::string& name, const std::string& value);

    /**
     * @brief 获取环境变量数组（供execve使用）
     *
     * @return 环境变量数组，以NULL结尾
     */
    char** getEnvArray();

    /**
     * @brief 清理所有环境变量
     */
    void clear();

    /**
     * @brief 获取环境变量数量
     *
     * @return 环境变量数量
     */
    size_t getVarCount() const { return envStrings_.size(); }

    /**
     * @brief 打印所有环境变量（调试用）
     */
    void printEnvironment() const;

private:
    std::vector<std::string> envStrings_;  // 环境变量字符串
    std::vector<char*> envArray_;          // 环境变量数组
    std::map<std::string, std::string> envMap_;  // 环境变量映射（防重复）

    /**
     * @brief 添加标准CGI环境变量
     *
     * @param request HTTP请求对象
     * @param scriptPath 脚本路径
     */
    void addStandardVars(const HttpRequest& request, const std::string& scriptPath);

    /**
     * @brief 添加服务器相关环境变量
     */
    void addServerVars();

    /**
     * @brief 添加请求相关环境变量
     *
     * @param request HTTP请求对象
     */
    void addRequestVars(const HttpRequest& request);

    /**
     * @brief 构建环境变量数组
     */
    void buildEnvArray();

    /**
     * @brief 添加单个环境变量
     *
     * @param name 变量名
     * @param value 变量值
     */
    void addVar(const std::string& name, const std::string& value);

    /**
     * @brief 安全地转换数字为字符串
     *
     * @param value 数字值
     * @return 字符串
     */
    template<typename T>
    std::string toString(T value);
};

#endif // CGI_ENVIRONMENT_HPP