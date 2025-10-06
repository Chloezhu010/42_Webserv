#include "cgi_process.hpp"
#include <unistd.h>
#include <sys/wait.h>
#include <sys/select.h>
#include <signal.h>
#include <cstring>
#include <iostream>
#include <ctime>
#include <sys/time.h>

CGIProcess::CGIProcess() : childPid_(-1), pipesCreated_(false) {
    inputPipe_[0] = inputPipe_[1] = -1;
    outputPipe_[0] = outputPipe_[1] = -1;
}

CGIProcess::~CGIProcess() {
    killProcess();
    closePipes();
}

bool CGIProcess::execute(const std::string& cgiPath,
                        const std::string& scriptPath,
                        char** envp,
                        const std::string& inputData,
                        std::string& output,
                        int timeoutSeconds) {
    std::cout << "🔧 CGI: Starting execution..." << std::endl;
    std::cout << "🔧 CGI: Path: " << cgiPath << std::endl;
    std::cout << "🔧 CGI: Script: " << scriptPath << std::endl;
    std::cout << "🔧 CGI: Timeout: " << timeoutSeconds << "s" << std::endl;

    lastError_.clear();

    // 创建管道
    std::cout << "🔧 CGI: Creating pipes..." << std::endl;
    if (!createPipes()) {
        std::cout << "❌ CGI: Failed to create pipes" << std::endl;
        return false;
    }
    std::cout << "✅ CGI: Pipes created successfully" << std::endl;

    // Fork子进程
    std::cout << "🔧 CGI: Forking child process..." << std::endl;
    childPid_ = fork();
    if (childPid_ == -1) {
        std::cout << "❌ CGI: Fork failed" << std::endl;
        setError("Failed to fork process");
        closePipes();
        return false;
    }

    if (childPid_ == 0) {
        // 子进程：执行CGI
        std::cerr << "🔧 CGI Child: Setting up child process..." << std::endl;
        setupChildProcess(cgiPath, scriptPath, envp);
        // execve 不应该返回，如果返回说明出错
        std::cerr << "❌ CGI Child: execve failed, exiting" << std::endl;
        exit(1);
    } else {
        // 父进程：处理I/O
        std::cout << "🔧 CGI Parent: Child PID: " << childPid_ << std::endl;
        std::cout << "🔧 CGI Parent: Handling parent process..." << std::endl;
        return handleParentProcess(inputData, output, timeoutSeconds);
    }
}

bool CGIProcess::createPipes() {
    if (pipe(inputPipe_) == -1 || pipe(outputPipe_) == -1) {
        setError("Failed to create pipes");
        return false;
    }
    pipesCreated_ = true;
    return true;
}

void CGIProcess::closePipes() {
    if (pipesCreated_) {
        if (inputPipe_[0] != -1) close(inputPipe_[0]);
        if (inputPipe_[1] != -1) close(inputPipe_[1]);
        if (outputPipe_[0] != -1) close(outputPipe_[0]);
        if (outputPipe_[1] != -1) close(outputPipe_[1]);
        pipesCreated_ = false;
    }
}

bool CGIProcess::setupChildProcess(const std::string& cgiPath,
                                  const std::string& scriptPath,
                                  char** envp) {
    std::cout << "🔧 CGI Child: Redirecting stdin/stdout..." << std::endl;
    // 重定向stdin和stdout
    dup2(inputPipe_[0], STDIN_FILENO);
    dup2(outputPipe_[1], STDOUT_FILENO);
    // dup2(outputPipe_[1], STDERR_FILENO);

    std::cout << "🔧 CGI Child: Closing unused pipe ends..." << std::endl;
    // 关闭不需要的管道端
    close(inputPipe_[1]);
    close(outputPipe_[0]);
    close(inputPipe_[0]);
    close(outputPipe_[1]);

    // 不切换目录，直接使用完整路径执行脚本
    std::cout << "🔧 CGI Child: Using full script path: " << scriptPath << std::endl;

    // 执行CGI程序
    char* argv[] = {
        const_cast<char*>(cgiPath.c_str()),
        const_cast<char*>(scriptPath.c_str()),
        NULL
    };

    std::cerr << "🔧 CGI Child: Executing: " << cgiPath << " " << scriptPath << std::endl;
    execve(cgiPath.c_str(), argv, envp);
    std::cerr << "❌ CGI Child: Failed to execute CGI: " << cgiPath << std::endl;
    return false;
}

bool CGIProcess::handleParentProcess(const std::string& inputData,
                                    std::string& output,
                                    int timeoutSeconds) {
    std::cout << "🔧 CGI Parent: Closing child's pipe ends..." << std::endl;
    // 关闭子进程使用的管道端
    close(inputPipe_[0]);
    close(outputPipe_[1]);

    // 发送输入数据
    std::cout << "🔧 CGI Parent: Sending input data (" << inputData.length() << " bytes)..." << std::endl;
    if (!inputData.empty()) {
        writeToPipe(inputPipe_[1], inputData);
    }
    close(inputPipe_[1]);

    // 读取输出
    std::cout << "🔧 CGI Parent: Reading output from child..." << std::endl;
    bool success = readFromPipe(outputPipe_[0], output, timeoutSeconds);
    std::cout << "🔧 CGI Parent: Read " << output.length() << " bytes from child" << std::endl;
    close(outputPipe_[0]);

    // 等待子进程结束
    std::cout << "🔧 CGI Parent: Waiting for child to finish..." << std::endl;
    if (!waitForChild(timeoutSeconds)) {
        std::cout << "❌ CGI Parent: Child timeout or error, killing child" << std::endl;
        killChild();
        setError("CGI process timeout or error");
        return false;
    }

    std::cout << "✅ CGI Parent: Child finished successfully" << std::endl;
    return success;
}

bool CGIProcess::readFromPipe(int fd, std::string& output, int timeoutSeconds) {
    output.clear();
    char buffer[4096];

    fd_set readSet;
    struct timeval timeout;
    timeout.tv_sec = timeoutSeconds;
    timeout.tv_usec = 0;

    while (true) {
        FD_ZERO(&readSet);
        FD_SET(fd, &readSet);

        int result = select(fd + 1, &readSet, NULL, NULL, &timeout);
        if (result == 0) {
            // 超时
            break;
        }
        if (result < 0) {
            setError("select() failed during CGI output reading");
            return false;
        }

        ssize_t bytesRead = read(fd, buffer, sizeof(buffer));
        if (bytesRead <= 0) {
            break; // EOF或错误
        }

        output.append(buffer, bytesRead);
    }

    return true;
}

bool CGIProcess::writeToPipe(int fd, const std::string& data) {
    size_t totalWritten = 0;
    while (totalWritten < data.size()) {
        ssize_t bytesWritten = write(fd, data.c_str() + totalWritten,
                                   data.size() - totalWritten);
        if (bytesWritten <= 0) {
            return false;
        }
        totalWritten += bytesWritten;
    }
    return true;
}

bool CGIProcess::waitForChild(int timeoutSeconds) {
    int status;

    // 使用循环和短暂的非阻塞检查，而不是sleep
    time_t startTime = time(NULL);
    time_t currentTime;

    std::cout << "🔧 CGI Parent: Waiting for child PID " << childPid_ << " (timeout=" << timeoutSeconds << "s)" << std::endl;

    do {
        pid_t result = waitpid(childPid_, &status, WNOHANG);
        currentTime = time(NULL);

        if (result == childPid_) {
            // 子进程已结束
            std::cout << "🔧 CGI Parent: Child exited, status=" << status << std::endl;
            std::cout << "🔧 CGI Parent: WIFEXITED=" << WIFEXITED(status) << ", WEXITSTATUS=" << WEXITSTATUS(status) << std::endl;
            childPid_ = -1;
            return WIFEXITED(status) && WEXITSTATUS(status) == 0;
        } else if (result == -1) {
            // waitpid 出错
            std::cout << "❌ CGI Parent: waitpid error" << std::endl;
            childPid_ = -1;
            return false;
        }

        // 子进程还在运行，检查超时
        if (currentTime - startTime >= timeoutSeconds) {
            // 超时
            std::cout << "❌ CGI Parent: Timeout after " << (currentTime - startTime) << "s" << std::endl;
            return false;
        }

        if ((currentTime - startTime) % 2 == 0 && (currentTime - startTime) > 0) {
            std::cout << "🔧 CGI Parent: Still waiting... elapsed=" << (currentTime - startTime) << "s" << std::endl;
        }

        // 短暂等待，避免忙等待
        usleep(10000); // 等待10毫秒

    } while (true);
}

void CGIProcess::killChild() {
    if (childPid_ > 0) {
        kill(childPid_, SIGTERM);
        sleep(1);
        if (waitpid(childPid_, NULL, WNOHANG) == 0) {
            kill(childPid_, SIGKILL);
            waitpid(childPid_, NULL, 0);
        }
        childPid_ = -1;
    }
}

void CGIProcess::killProcess() {
    killChild();
}

std::string CGIProcess::getScriptDirectory(const std::string& scriptPath) {
    size_t lastSlash = scriptPath.find_last_of('/');
    if (lastSlash == std::string::npos) {
        return ".";
    }
    return scriptPath.substr(0, lastSlash);
}

void CGIProcess::setError(const std::string& error) {
    lastError_ = error;
}