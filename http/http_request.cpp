#include "http_request.hpp"

/* check if the method is valid */
static bool isValidMethod(const std::string& method)
{
    if (method == "GET" || method == "POST" || method == "DELETE")
        return true;
    return false;
}

/* extract the method from the request line
    - return empty string if not found
    - return GET, POST, DELETE etc, if found
*/
static std::string extractMethod(const std::string& request_buffer)
{
    // find the first space
    size_t first_space = request_buffer.find(' ');
    // if no space found, return empty string
    if (first_space == std::string::npos)
        return "";
    // extract the method
    std::string method = request_buffer.substr(0, first_space);
    // return the method
    return method;
}

/* only POST can have body, GET and DELETE cann't */
bool HttpRequest::methodCanHaveBody(const std::string& method) const
{
    return (method == "POST");
}

/* extract the content length from the header section
    - return -1 if not found or invalid
    - return the content length if found
*/
size_t HttpRequest::extractContentLength(const std::string& header_section) const
{

}

bool HttpRequest::isRequestComplete(const std::string& request_buffer) const {
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
    std::string header_section = request_buffer.substr(0, header_end);
    size_t content_length = extractContentLength(header_section)




    return true;
}

