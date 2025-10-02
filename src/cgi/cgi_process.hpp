#ifndef CGI_PROCESS_HPP
#define CGI_PROCESS_HPP

#include <string>
#include <sys/types.h>
#include <stdlib.h>

/**
 * @brief CGI进程管理器
 *
 * 负责fork子进程、执行CGI程序、管理进程间通信
 * 处理进程超时、错误恢复等
 */
class CGIProcess {
public:
    /**
     * @brief 构造函数
     */
    CGIProcess();

    /**
     * @brief 析构函数
     */
    ~CGIProcess();

    /**
     * @brief 执行CGI进程
     *
     * @param cgiPath CGI程序路径（如 /usr/bin/python3）
     * @param scriptPath 脚本文件路径（如 ./www/test.py）
     * @param envp 环境变量数组
     * @param inputData 输入数据（POST body等）
     * @param output 输出数据（CGI程序的输出）
     * @param timeoutSeconds 超时时间（秒）
     * @return true 执行成功，false 执行失败
     */
    bool execute(const std::string& cgiPath,
                 const std::string& scriptPath,
                 char** envp,
                 const std::string& inputData,
                 std::string& output,
                 int timeoutSeconds = 30);

    /**
     * @brief 获取最后的错误信息
     *
     * @return 错误描述字符串
     */
    const std::string& getLastError() const { return lastError_; }

    /**
     * @brief 检查是否有进程正在运行
     *
     * @return true 有进程运行，false 无进程运行
     */
    bool isRunning() const { return childPid_ > 0; }

    /**
     * @brief 强制终止当前CGI进程
     */
    void killProcess();

private:
    std::string lastError_;     // 最后的错误信息
    pid_t childPid_;           // 子进程PID
    int inputPipe_[2];         // 输入管道（父进程写，子进程读）
    int outputPipe_[2];        // 输出管道（子进程写，父进程读）
    bool pipesCreated_;        // 管道是否已创建

    /**
     * @brief 创建进程间通信管道
     *
     * @return true 创建成功，false 创建失败
     */
    bool createPipes();

    /**
     * @brief 关闭所有管道
     */
    void closePipes();

    /**
     * @brief 设置子进程环境
     *
     * @param cgiPath CGI程序路径
     * @param scriptPath 脚本路径
     * @param envp 环境变量
     * @return true 设置成功，false 设置失败
     */
    bool setupChildProcess(const std::string& cgiPath,
                          const std::string& scriptPath,
                          char** envp);

    /**
     * @brief 处理父进程逻辑
     *
     * @param inputData 要发送给子进程的数据
     * @param output 从子进程读取的输出
     * @param timeoutSeconds 超时时间
     * @return true 处理成功，false 处理失败
     */
    bool handleParentProcess(const std::string& inputData,
                            std::string& output,
                            int timeoutSeconds);

    /**
     * @brief 等待子进程结束（带超时）
     *
     * @param timeoutSeconds 超时时间
     * @return true 子进程正常结束，false 超时或错误
     */
    bool waitForChild(int timeoutSeconds);

    /**
     * @brief 强制杀死子进程
     */
    void killChild();

    /**
     * @brief 设置错误信息
     *
     * @param error 错误描述
     */
    void setError(const std::string& error);

    /**
     * @brief 从管道读取所有数据
     *
     * @param fd 文件描述符
     * @param output 输出字符串
     * @param timeoutSeconds 超时时间
     * @return true 读取成功，false 读取失败
     */
    bool readFromPipe(int fd, std::string& output, int timeoutSeconds);

    /**
     * @brief 向管道写入数据
     *
     * @param fd 文件描述符
     * @param data 要写入的数据
     * @return true 写入成功，false 写入失败
     */
    bool writeToPipe(int fd, const std::string& data);

    /**
     * @brief 获取脚本所在目录
     *
     * @param scriptPath 脚本完整路径
     * @return 目录路径
     */
    std::string getScriptDirectory(const std::string& scriptPath);

    // 禁止拷贝构造和赋值
    CGIProcess(const CGIProcess&);
    CGIProcess& operator=(const CGIProcess&);
};

#endif // CGI_PROCESS_HPP