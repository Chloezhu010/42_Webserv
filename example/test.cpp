#include <string>
#include <iostream>
#include <algorithm>
#include <sstream>

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
    // std::string request_buffer = 
    //     "G@T / HTTP/1.1\r\nHost: localhost:8080\r\ncontent-length: 11\r\n";
    // std::string method = extractMethod(request_buffer);
    // if (method.empty())
    //     std::cout << "Error: Method not found" << std::endl;
    // else
    //     std::cout << "Method: " << method << std::endl;

    std::string header_section = "content-length: 11a\n";
    size_t pos = header_section.find("content-length:");
    size_t colon_pos = header_section.find(':', pos);
    size_t line_end = header_section.find("\n", colon_pos);
    std::string length_str = header_section.substr(colon_pos + 1, line_end - colon_pos - 1);
    std::cout << "colon_pos: " << colon_pos
                << "; line_end: " << line_end
                << "; leng_str: " << length_str << std::endl;
    size_t start = length_str.find_first_not_of(" \t\r\n");
    size_t end = length_str.find_last_not_of(" \t\r\n");
    length_str = length_str.substr(start, end - start + 1);
    std::cout << "trimmed leng_str: '" << length_str << "'" << std::endl;
    // long content_length;
    // std::istringstream iss(length_str);
    // iss >> content_length;
    // if (iss.fail())
    // {
    //     content_length = -1;
    //     std::cout << "Error: Invalid Content-Length" << std::endl;
    // }
    // else
    //     std::cout << "content_length: " << content_length << std::endl;
    char *endptr;
    long content_length = strtol(length_str.c_str(), &endptr, 10);
    std::cout << "endptr: " << (int)(*endptr) << std::endl;

    return 0;
}