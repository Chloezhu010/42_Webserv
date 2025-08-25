#include <string>
#include <iostream>

static bool isValidMethod(const std::string& method)
{
    if (method == "GET" || method == "POST" || method == "DELETE")
        return true;
    return false;
}

static std::string extractMethod(const std::string& request_buffer)
{
    // find the first space
    size_t first_space = request_buffer.find(' ');
    // if no space found, return empty string
    if (first_space == std::string::npos)
        return "";
    // extract the method
    std::string method = request_buffer.substr(0, first_space);
    // method should be all uppercase
    if (!isValidMethod(method))
        return "";
    return method;
}

int main()
{
    std::string request_buffer = "G@T / HTTP/1.1\r\nHost: localhost:8080\r\n";
    std::string method = extractMethod(request_buffer);
    if (method.empty())
        std::cout << "Error: Method not found" << std::endl;
    else
        std::cout << "Method: " << method << std::endl;
    return 0;
}