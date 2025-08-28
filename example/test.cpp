#include <string>
#include <iostream>
#include <algorithm>
#include <sstream>

// constants (TBD)
const size_t MAX_REQUEST_SIZE = 8*1024*1024;
const size_t MAX_HEADER_SIZE =8*1024;
const size_t MAX_HEADER = 100;
const size_t MAX_URI_LENGTH = 2048;
const size_t MAX_BODY_SIZE = 10*1024*1024;

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

enum RequestStatus {
    NEED_MORE_DATA, //0
    REQUEST_COMPLETE, //1
    REQUEST_TOO_LARGE, //2
    INVALID_REQUEST //3
};

RequestStatus checkBodyComplete(const std::string& buffer, size_t header_end) {
    /* extract the header section */
    std::string header_section = buffer.substr(0, header_end);
    
    /* extract content length */
    long content_length = extractContentLength(header_section);
    if (content_length < 0) // invalid content-length
        return INVALID_REQUEST;
    if (content_length == 0) // no body needed
        return REQUEST_COMPLETE;
    if (content_length > MAX_BODY_SIZE) // exceed body size
        return REQUEST_TOO_LARGE;
    
    /* check actual body vs content-length */
    size_t body_start = header_end + 4;
    size_t body_length = buffer.length() - body_start;
    if (body_length < content_length)
        return NEED_MORE_DATA;
    // else if (body_length >= content_length)
    return REQUEST_COMPLETE;
}

/* initial check if the http request is complete */
RequestStatus isRequestComplete(const std::string& request_buffer) {
    // check if the header is complete
    size_t header_end = request_buffer.find("\r\n\r\n");
    if (header_end == std::string::npos) {
        return NEED_MORE_DATA;
    }
    // extract the method from the first line
    std::string method = extractMethod(request_buffer);
    if (method.empty()) {
        return INVALID_REQUEST; // if cannot extract method, return false
    }
    if (!isValidMethod(method))
        return INVALID_REQUEST; // if invalid method, return false (not supported)
    // check if the method requires a body
    // for GET, DELETE that cannot have body, return true
    if (!methodCanHaveBody(method)) {
        return REQUEST_COMPLETE;
    }
    // for POST that can have body, check if Content-Length header is present
    std::string header_section = request_buffer.substr(0, header_end + 4); // keep "\r\n\r\n"
    long content_length = extractContentLength(header_section);
    if (content_length < 0)
        return INVALID_REQUEST; // if invalid content length, return false
    // check if received body length is equal to content length
    size_t body_start = header_end + 4; // after "\r\n\r\n"
    size_t received_body_length = request_buffer.length() - body_start;
    if (received_body_length < static_cast<size_t>(content_length))
        return NEED_MORE_DATA;
    else
        return REQUEST_COMPLETE;
}

static size_t split_by_space(const std::string& str)
{
    int count = 1;

    size_t first_space = str.find(' ');
    if (first_space == std::string::npos)
        return 0;
    count++;
    size_t second_space = str.find(' ', first_space + 1);
    if (second_space == std::string::npos)
        return count;
    count++;
    size_t third_space = str.find(' ', second_space + 1);
    if (third_space == std::string::npos)
        return count;
    count++;
    return count;
}

int main()
{
    std::string request_buffer = 
        "GET / HTTP/1.1";
    std::string request_buffer2 = 
        "   ";
    // std::string request_buffer3 = 
    //     "POST / HTTP/1.1\r\nHost: localhost:8080\r\ncontent-length: 5\r\n\r\nhello=world";

    // size_t header_end = request_buffer2.find("\r\n\r\n");
    // size_t bodycomplete_status = checkBodyComplete(request_buffer2, header_end);
    // std::cout << "body status: " << bodycomplete_status << std::endl;

    // size_t request_status = isRequestComplete(request_buffer);
    // std::cout << "request status: " << request_status << std::endl;

    size_t split_count = split_by_space(request_buffer);
    std::cout << "split count: " << split_count << std::endl;
    size_t split_count2 = split_by_space(request_buffer2);
    std::cout << "split count2: " << split_count2 << std::endl;

    return 0;
}