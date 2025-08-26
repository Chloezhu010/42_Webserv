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
    return method;
}

long extractContentLength(const std::string& header_section)
{
    // convert header_section to lower case for case-insensitive search
    std::string lower_header = header_section;
    std::transform(lower_header.begin(), lower_header.end(), lower_header.begin(), ::tolower);

    // find "content-length:" in the header section
    size_t pos = lower_header.find("content-length:");
    if (pos == std::string::npos){
        return -1; // not found
    }

    // find the value after "content-length:"
    size_t colon_pos = lower_header.find(':', pos); // find the ':' after content-length
    size_t line_end = lower_header.find("\r\n", colon_pos); // find the end of the line
    if (colon_pos == std::string::npos || line_end == std::string::npos)
        return -1; // invalid format
    std::string length_str = lower_header.substr(colon_pos + 1, line_end - colon_pos - 1);

    // cleanup the value (remove spaces, \r, \n)
    size_t start = length_str.find_first_not_of(" \t\r\n");
    size_t end = length_str.find_last_not_of(" \t\r\n");
    if (start == std::string::npos || end == std::string::npos)
        return -1; // invalid format
    length_str = length_str.substr(start, end - start + 1);

    // convert the value to long
    char *endptr; // point to the 1st invalid character
    long content_length = strtol(length_str.c_str(), &endptr, 10);
    if (*endptr != 0 || content_length < 0)
        return -1; // invalid content length

    // return the content length
    return content_length;
}

bool methodCanHaveBody(const std::string& method)
{
    return (method == "POST");
}

bool isRequestComplete(const std::string& request_buffer) {
    // check if the header is complete
    size_t header_end = request_buffer.find("\r\n\r\n");
    if (header_end == std::string::npos) {
        return false;
    }
    // extract the method from the first line
    std::string method = extractMethod(request_buffer);
    if (method.empty()) {
        return false; // if cannot extract method, return false
    }
    if (!isValidMethod(method))
        return false; // if invalid method, return false (not supported)
    // check if the method requires a body
    // for GET, DELETE that cannot have body, return true
    if (!methodCanHaveBody(method)) {
        return true;
    }
    // for POST that can have body, check if Content-Length header is present
    std::string header_section = request_buffer.substr(0, header_end + 4);
    long content_length = extractContentLength(header_section);
    if (content_length < 0)
        return false; // if invalid content length, return false
    // check if received body length is equal to content length
    size_t body_start = header_end + 4; // after "\r\n\r\n"
    size_t received_body_length = request_buffer.length() - body_start;
    if (received_body_length < static_cast<size_t>(content_length))
        return false; // if received body length is less than content length, return false
    // if all checks pass, return true
    return true;
}

int main()
{
    std::string request_buffer = 
        "GET / HTTP/1.1\r\nHost: localhost:8080\r\n\r\n";
    std::string request_buffer2 = 
        "POST / HTTP/1.1\r\nHost: localhost:8080\r\ncontent-length: 11\r\nconnection: close\r\n\r\nhello=world\r\n\r\n";
    std::string request_buffer3 = 
        "POST / HTTP/1.1\r\nHost: localhost:8080\r\ncontent-length: 10\r\n\r\nhello=world\r\n\r\n";

    // std::string method = extractMethod(request_buffer);
    // if (method.empty())
    //     std::cout << "Error: Method not found" << std::endl;
    // else
    //     std::cout << "Method: " << method << std::endl;

    // std::string header_section = "content-length:   11   \r\n";
    // long content_length = extractContentLength(header_section);
    // if (content_length < 0)
    // {
    //     std::cout << "Error: Invalid Content-Length" << std::endl;
    //     // std::cout << "content_length: " << content_length << std::endl;
    // }
    // else
    //     std::cout << "Valid content_length: " << content_length << std::endl;
    
    size_t header_end = request_buffer2.find("\r\n\r\n");
    std::string header_section = request_buffer2.substr(0, header_end);
    std::cout << "Header section: '" << header_section << "'" << std::endl;
    std::cout << std::endl;
    long content_length2 = extractContentLength(header_section);
    std::cout << "content_length2: " << content_length2 << std::endl;

    // size_t body_start = header_end + 4;
    // size_t received_body_length = request_buffer2.length() - body_start;
    // std::cout << "received_body_length2: " << received_body_length << std::endl;

    // bool is_complete = isRequestComplete(request_buffer);
    // std::cout << "is_complete: " << is_complete << std::endl;
    bool is_complete2 = isRequestComplete(request_buffer2);
    std::cout << "is_complete: " << is_complete2 << std::endl;
    bool is_complete3 = isRequestComplete(request_buffer3);
    std::cout << "is_complete: " << is_complete3 << std::endl;

    return 0;
}