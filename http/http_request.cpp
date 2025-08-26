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
    - content-length can be 0, can return 200 ok
*/
long HttpRequest::extractContentLength(const std::string& header_section) const
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

    // cleanup the value (remove spaces, \t, \r, \n)
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

/* check if the http request is complete
    - return value
        - TODO: need to change to value <> for http response later
    - check if the header is complete: TODO to enhance "host"
    - check if the method is valid: currently only support GET, POST, DELETE
    - 
*/
RequestStatus HttpRequest::isRequestComplete(const std::string& request_buffer) const {
    // check if the header is complete
    size_t header_end = request_buffer.find("\r\n\r\n");
    if (header_end == std::string::npos) {
        return INCOMPLETE;
    }
    // extract the method from the first line
    std::string method = extractMethod(request_buffer);
    if (method.empty()) {
        return INVALID; // if cannot extract method, return false
    }
    if (!isValidMethod(method))
        return INVALID; // if invalid method, return false (not supported)
    // check if the method requires a body
    // for GET, DELETE that cannot have body, return true
    if (!methodCanHaveBody(method)) {
        return COMPLETE;
    }
    // for POST that can have body, check if Content-Length header is present
    std::string header_section = request_buffer.substr(0, header_end + 4); // keep "\r\n\r\n"
    long content_length = extractContentLength(header_section);
    if (content_length < 0)
        return INVALID; // if invalid content length, return false
    // check if received body length is equal to content length
    size_t body_start = header_end + 4; // after "\r\n\r\n"
    size_t received_body_length = request_buffer.length() - body_start;
    if (received_body_length < static_cast<size_t>(content_length))
        return INCOMPLETE;
    else if (received_body_length > static_cast<size_t>(content_length))
        return COMPLETE;
    else
        return OVERSIZED;
}

