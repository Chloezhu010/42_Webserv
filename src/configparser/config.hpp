#ifndef CONFIG_HPP
#define CONFIG_HPP

#include <string>
#include <vector>
#include <map>
#include <stdexcept>

// Location配置结构体
struct LocationConfig {
    std::string path;                        // location路径
    std::string root;                        // 文档根目录
    std::vector<std::string> index;          // 默认索引文件
    std::vector<std::string> allowMethods;   // 允许的HTTP方法
    bool autoindex;                          // 是否开启目录浏览
    std::string cgiExtension;                // CGI扩展名
    std::string cgiPath;                     // CGI程序路径
    std::string redirect;                    // 重定向URL
    
    // 默认构造函数
    LocationConfig() : autoindex(false) {}
    
    // 构造函数
    LocationConfig(const std::string& locationPath) 
        : path(locationPath), autoindex(false) {}
};

// Server配置结构体
struct ServerConfig {
    std::vector<int> listen;                 // 监听端口
    std::vector<std::string> serverName;     // 服务器名
    size_t clientMaxBodySize;                // 客户端最大请求体大小
    std::string root;                        // 服务器根目录
    std::vector<std::string> index;          // 默认索引文件
    std::map<int, std::string> errorPages;   // 错误页面映射
    std::vector<LocationConfig> locations;   // location配置列表
    
    // 默认构造函数
    ServerConfig() : clientMaxBodySize(1048576) {} // 默认1MB
    
    // 辅助函数：添加监听端口
    void addListenPort(int port) {
        listen.push_back(port);
    }
    
    // 辅助函数：添加服务器名
    void addServerName(const std::string& name) {
        serverName.push_back(name);
    }
    
    // 辅助函数：添加错误页面
    void addErrorPage(int errorCode, const std::string& page) {
        errorPages[errorCode] = page;
    }
    
    // 辅助函数：添加location配置
    void addLocation(const LocationConfig& location) {
        locations.push_back(location);
    }
};

// 全局配置结构体
struct Config {
    std::vector<ServerConfig> servers;       // 所有服务器配置
    
    // 默认构造函数
    Config() {}
    
    // 辅助函数：添加服务器配置
    void addServer(const ServerConfig& server) {
        servers.push_back(server);
    }
    
    // 辅助函数：获取服务器数量
    size_t getServerCount() const {
        return servers.size();
    }
    
    // 辅助函数：根据索引获取服务器配置
    const ServerConfig& getServer(size_t index) const {
        if (index >= servers.size()) {
            throw std::out_of_range("Server index out of range");
        }
        return servers[index];
    }
    
    // 辅助函数：清空配置
    void clear() {
        servers.clear();
    }
    
    // 辅助函数：检查配置是否为空
    bool empty() const {
        return servers.empty();
    }
};

void displayServerConfig(const ServerConfig& server, size_t serverIndex = 0);
void displayLocationConfig(const LocationConfig& location, size_t indent = 0);
void displayFullConfig(const Config& config);

#endif // CONFIG_HPP