#include "http_request.hpp"

// ============================================================================
// Constructors & Destructors                                                  
// ============================================================================
    
HttpRequest::HttpRequest(): is_complete_(false), is_valid_(false),
    validation_error_(NOT_VALIDATED), content_length_(-1), chunked_encoding_(false)
{}

HttpRequest::~HttpRequest()
{}


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

/* helper function: check if header contains chunked encoding 
    - return true if found, false otherwise
*/
static bool hasChunkedEncoding(const std::string& header_section)
{
    // convert header_section to lower case for case-insensitive search
    std::string lower_header = header_section;
    std::transform(lower_header.begin(), lower_header.end(), lower_header.begin(), ::tolower);
    // find "transfer-encoding:" in the header section
    size_t pos = lower_header.find("transfer-encoding:");
    if (pos == std::string::npos)
        return false; // not found
    // find the value after "transfer-encoding:"
    size_t line_end = lower_header.find("\r\n", pos); // find the end of the line
    if (line_end == std::string::npos)
        return false; // invalid format
    // extract the value after "transfer-encoding:"
    std::string te_value = lower_header.substr(pos, line_end - pos);
    // if value contains chunked and chunked is the last word
    return (te_value.find("chunked") != std::string::npos);
}

/* helper function: basic check if the chunked body is complete
    - return REQUEST_COMPLETE if complete
    - return NEED_MORE_DATA if not complete
*/
RequestStatus HttpRequest::isChunkedBodyComplete(const std::string& request_buffer, size_t header_end) const {
    size_t body_start = header_end + 4; // after "\r\n\r\n"
    // extract the body part
    std::string body_part = request_buffer.substr(body_start);
    // look for final chunk
    size_t final_chunk_pos = body_part.find("0\r\n");
    if (final_chunk_pos == std::string::npos)
        return NEED_MORE_DATA;
    // check if the final chunk is followed by "\r\n\r\n"
    size_t trailer_pos = body_part.find("\r\n\r\n", final_chunk_pos);
    if (trailer_pos != std::string::npos)
        return REQUEST_COMPLETE;
    return NEED_MORE_DATA;
}

/* helper function: basic check if the content-length body is complete
    - return REQUEST_COMPLETE if complete
    - return NEED_MORE_DATA if not complete
*/
RequestStatus HttpRequest::isContentLengthBodyComplete(const std::string& request_buffer, size_t header_end, long content_length) const
{
    size_t body_start = header_end + 4; // after "\r\n\r\n"
    size_t received_body_length = request_buffer.length() - body_start;
    if (received_body_length < static_cast<size_t>(content_length))
        return NEED_MORE_DATA;
    return REQUEST_COMPLETE;
}

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
    // 1. check if the header is complete
    size_t header_end = request_buffer.find("\r\n\r\n");
    if (header_end == std::string::npos) {
        return NEED_MORE_DATA;
    }
    // 2. basic method check
    std::string method = extractMethod(request_buffer);
    if (method.empty()) {
        return INVALID_REQUEST; // if cannot extract method, return false
    }
    if (!isValidMethod(method))
        return INVALID_REQUEST; // if invalid method, return false (not supported)
    // 3. check if the method requires a body
    // for GET, DELETE that cannot have body, return true
    if (!methodCanHaveBody(method)) {
        return REQUEST_COMPLETE;
    }
    // 4. for POST that can have body
    std::string header_section = request_buffer.substr(0, header_end + 4); // keep "\r\n\r\n"
    // 5. check for conflicting headers
    bool has_chunked_encoding = hasChunkedEncoding(header_section);
    bool has_content_length = extractContentLength(header_section) >= 0;
    if (has_chunked_encoding && has_content_length)
        return INVALID_REQUEST; // conflicting headers
    // 6. if chunked, check if the body is complete
    if (hasChunkedEncoding(header_section))
        // 6a. check if the chunked body is complete
        return isChunkedBodyComplete(request_buffer, header_end);
    else
    {
         // 7. if not chunked, check content-length
        long content_length = extractContentLength(header_section);
        if (content_length < 0)
            return INVALID_REQUEST; // if invalid content length, return false
        // 7a. check if received body length is equal to content length
        return isContentLengthBodyComplete(request_buffer, header_end, content_length);
    }
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
                && (clean_line[clean_line.length() - 1] == '\r' || clean_line[clean_line.length() - 1] == '\n')) {
        clean_line.erase(clean_line.length() - 1);
    }
    // check for leading/ trailing space or empty line
    if (clean_line.empty() || clean_line[0] == ' ' || clean_line[clean_line.length() - 1] == ' ')
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

// helper function: split the header section into lines
static std::vector<std::string> splitIntoLines(const std::string& str)
{
    std::vector<std::string> results;
    // if the string is empty, return empty vector
    if (str.empty())
        return results;
    // split by \r\n, handle cases with only \n or only \r
    size_t start = 0;
    size_t end = 0;
    while (end < str.length())
    {
        // standard line ending \r\n
        if (str[end] == '\r' && end + 1 < str.length() && str[end + 1] == '\n')
        {
            results.push_back(str.substr(start, end - start));
            end += 2; // skip \r\n
            start = end;
        }
        // handle lone \n
        else if (str[end] == '\n') 
        {
            results.push_back(str.substr(start, end - start));
            end += 1; // skip \n
            start = end;
        }
        // handle lone \r
        else if (str[end] == '\r')
        {
            results.push_back(str.substr(start, end - start));
            end += 1; // skip \r
            start = end;
        }
        else
            end++;
    }
    // add the last line if any
    if (start < str.length())
        results.push_back(str.substr(start));
    
    return results;
}

/* parse the header section
    - header section format: (header-name ":" OWS header-value OWS CRLF)* 
    - return true if parsed successfully, false otherwise
    - name-value pairs stored in headers_ map
*/
bool HttpRequest::parseHeaders(const std::string& header_section)
{
    // check for empty header section
    if (header_section.empty())
        return true; // no header is valid
    
    // split the header section by line
    std::vector<std::string> lines = splitIntoLines(header_section);
    std::string line;
    for (size_t i = 0; i < lines.size(); i++)
    {
        line = lines[i];
        
        // empty line terminates header section
        if (line.empty())
            break;
        
        // check header size limit
        if (line.length() > MAX_HEADER_SIZE)
            return false; // header line too long
        
        // find colon pos
        size_t colon_pos = line.find(':');
        if (colon_pos == std::string::npos)
            return false; // no colon found, invalid header
        
        // extract header name and value
        std::string name = line.substr(0, colon_pos);
        std::string value;
        if (colon_pos + 1 < line.length())
            value = line.substr(colon_pos + 1);
        else
            value = "";
        
        // trim leading/trailing spaces & tabs from name & value
        size_t name_start = name.find_first_not_of(" \t\r\n");
        size_t name_end = name.find_last_not_of(" \t\r\n");
        if (name_start == std::string::npos || name_end == std::string::npos)
            return false; // invalid name
        name = name.substr(name_start, name_end - name_start + 1);    
        size_t value_start = value.find_first_not_of(" \t\r\n");
        size_t value_end = value.find_last_not_of(" \t\r\n");
        if (value_start == std::string::npos || value_end == std::string::npos)
            value = ""; // empty value is valid    
        else
            value = value.substr(value_start, value_end - value_start + 1);
        // check for empty name
        if (name.empty())
            return false; // empty name is invalid
            
        // convert name to lower case for case-insensitive comparison
        std::transform(name.begin(), name.end(), name.begin(), ::tolower);
        // store the header in the map
        headers_[name] = value;
        // check header count limit
        if (headers_.size() > MAX_HEADER_COUNT)
            return false; // too many headers       
    }
    
    return true;
}

