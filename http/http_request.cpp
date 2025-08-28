#include "http_request.hpp"

// ============================================================================
// Extraction methods                                                  
// ============================================================================  

/* extract the method from the request line
    - return empty string if not found
    - return GET, POST, DELETE etc, if found
*/
std::string HttpRequest::extractMethod(const std::string& request_buffer) const
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

/* check if the method is valid */
bool HttpRequest::isValidMethod(const std::string& method) const
{
    if (method == "GET" || method == "POST" || method == "DELETE")
        return true;
    return false;
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

// ============================================================================
// Phase 1 Completeness check                                                  
// ============================================================================

/* check if the http request is complete
    - basic check: header completeness
    - basic method check: currently only support GET, POST, DELETE
    - check if method can have body
        - if GET, DELETE: can't have body
        - if POST: can have body
            - check content-legnth
            - validation of content-length <> body
    - return value: 
        - NEED_MORE_DATA 0
        - REQUEST_COMPLETE 1
        - REQUEST_TOO_LARGE 2
        - INVALID_REQUEST 3
*/
RequestStatus HttpRequest::isRequestComplete(const std::string& request_buffer) const {
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

// ============================================================================
// Phase 2 Parsing                                                  
// ============================================================================

/* helper function: split the request line by space
    - return the number of parts, split by space
*/
static std::vector<std::string> split_by_space(const std::string& str)
{
    // vector container to store the results
    std::vector<std::string> results;
    // if the string is empty, return empty vector
    if (str.empty())
        return results;
    std::istringstream iss(str);
    std::string token;
    while (iss >> token) {
        results.push_back(token);
    }
    return results;
}

/* parse the request line
    - request line format: METHOD SP URI SP HTTP/VERSION CRLF
    - return true if parsed successfully, false otherwise
*/
bool HttpRequest::parseRequestLine(const std::string& request_line)
{
    // cleanup trailing \r\n if any
    std::string clean_line = request_line;
    while (!clean_line.empty()
                && (clean_line.back() == '\r' || clean_line.back() == '\n')) {
        clean_line.pop_back();
    }
    // check for leading/ trailing space or empty line
    if (clean_line.empty() || clean_line.front() == ' ' || clean_line.back() == ' ')
        return false;

    // split the request line by space
    std::vector<std::string> parts = split_by_space(clean_line);
    if (parts.size() != 3)
        return false; // must be exactly 3 parts
    
    // check for empty parts
    for (size_t i = 0; i < parts.size(); i++) {
        if (parts[i].empty())
            return false; // empty part found
    }
      
    // extract method
    method_str_ = parts[0];

    // extract url and query string
    full_uri_ = parts[1];
    size_t query_pos = full_uri_.find('?');
    if (query_pos != std::string::npos) {
        uri_ = full_uri_.substr(0, query_pos);
        query_string_ = full_uri_.substr(query_pos + 1);
    } else {
        uri_ = full_uri_;
        query_string_ = "";
    }

    // extract http version
    http_version_ = parts[2];

    return true;
}