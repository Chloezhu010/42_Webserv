#include "http_request.hpp"

// ============================================================================
// Constructors & Destructors                                                  
// ============================================================================
    
HttpRequest::HttpRequest(): is_complete_(false), is_parsed_(false),
    validation_status_(NOT_VALIDATED), content_length_(-1), chunked_encoding_(false)
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
RequestStatus HttpRequest::isChunkedBodyComplete(const std::string& request_buffer, size_t header_end) {
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
RequestStatus HttpRequest::isContentLengthBodyComplete(const std::string& request_buffer, size_t header_end, long content_length)
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
    - set is_complete_ flag if REQUEST_COMPLETE
*/
RequestStatus HttpRequest::isRequestComplete(const std::string& request_buffer) {
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
    // if (!isValidMethod(method))
    //     return INVALID_REQUEST; // check in the validation phase later

    // 3. check if the method requires a body
    // for GET, DELETE that cannot have body, return true
    if (!methodCanHaveBody(method)) {
        is_complete_ = true; // set complete flag
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
    {
        // 6a. check if the chunked body is complete
        RequestStatus result = isChunkedBodyComplete(request_buffer, header_end);
        if (result == REQUEST_COMPLETE)
            is_complete_ = true; // set complete flag
        return result;
    }
    else
    {
        // 7. if not chunked, check content-length
        long content_length = extractContentLength(header_section);
        if (content_length < 0)
            return REQUEST_COMPLETE; // if invalid content length, allow complete, validate later
        // 7a. check if received body length is equal to content length
        RequestStatus result = isContentLengthBodyComplete(request_buffer, header_end, content_length);
        if (result == REQUEST_COMPLETE)
            is_complete_ = true; // set complete flag
        return result;
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
    if (full_uri_.length() > MAX_URI_LENGTH)
        return false; // uri too long
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
    - name-value pairs stored in headers_ multimap
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
        // store the header in the multimap
        headers_.insert(std::make_pair(name, value));
        // check header count limit
        if (headers_.size() > MAX_HEADER_COUNT)
            return false; // too many headers
    }
    // set flags: transfer-encoding & content-length & connection
    std::multimap<std::string, std::string>::iterator te_it = headers_.find("transfer-encoding");
    if (te_it != headers_.end())
    {
        std::string te_value = te_it->second;
        std::transform(te_value.begin(), te_value.end(), te_value.begin(), ::tolower);
        if (te_value.find("chunked") != std::string::npos)
            chunked_encoding_ = true;
    }
    std::multimap<std::string, std::string>::iterator cl_it = headers_.find("content-length");
    if (cl_it != headers_.end())
    {
        char *endptr;
        long cl = strtol(cl_it->second.c_str(), &endptr, 10);
        if (*endptr != 0 || cl < 0)
            content_length_ = -1;
        else
            content_length_ = cl;
    }
    std::multimap<std::string, std::string>::iterator connection_it = headers_.find("connection");
    if (connection_it != headers_.end())
    {
        connection_str_ = connection_it->second; // update the connection value str
        std::transform(connection_str_.begin(), connection_str_.end(), connection_str_.begin(), ::tolower);
        if (connection_str_.find("close") != std::string::npos)
            keep_alive_ = false; 
    }
    // mandatory host in header
    std::multimap<std::string, std::string>::iterator host_it = headers_.find("host");
    if (host_it == headers_.end())
        return false; // missing required host header  
    
    return true;
}

/* helper function: parse hex to decimal
    @return: the decimal value, -1 if invalid
*/
static long parseHexToDecimal(const std::string& chunk_size_str)
{
    // check if the hex line is empty
    if (chunk_size_str.empty())
        return -1;
    // convert hex to decimal
    size_t result = 0;
    for (size_t i = 0; i < chunk_size_str.length(); i++)
    {
        char c = chunk_size_str[i];
        if (c >= '0' && c <= '9')
            result = result * 16 + c - '0';
        else if (c >= 'a' && c <= 'f')
            result = result * 16 + c - 'a' + 10;
        else if (c >= 'A' && c <= 'F')
            result = result * 16 + c - 'A' + 10;
        else
            return -1; // invalid hex character
    }
    // convert size_t to long
    return static_cast<long>(result);
}

/* helper function: parse the chunked encoding body */
bool HttpRequest::decodeChunkedBody(const std::string& body_section)
{
// 1. clear the state
    body_.clear();
    size_t position = 0;
// 2. loop through the body section to decode chunked data
    while (position < body_section.length())
    {
        // parse chunked size line
        size_t crlf_pos = body_section.find("\r\n", position);
        if (crlf_pos == std::string::npos)
            return false;
        long size = parseHexToDecimal(body_section.substr(position, crlf_pos - position));
        if (size < 0)
            return false;
        position = crlf_pos + 2; // skip line + \r\n
        // handle final chunk
        if (size == 0)
        {
            // check if there is immediate "\r\n"
            if (position + 2 <= body_section.length() && body_section.substr(position, 2) == "\r\n")
                return true; // 0\r\n\r\n case
            // otherwise, check for final \r\n\r\n after trailing info
            size_t final_end = body_section.find("\r\n\r\n", position);
            return (final_end != std::string::npos);
        }
        // handle regular chunk
        // check data availability
        if (position + size + 2 > body_section.length())
            return false;
        // extract and append chunk data
        body_.append(body_section.substr(position, size));
        position += size;
        // validate chunk ending
        if (body_section.substr(position, 2) != "\r\n")
            return false;
        position += 2; // move past \r\n

        // check against max chunk size
        if (body_.length() > MAX_BODY_SIZE)
            return false;
    }
    return false;
}

/* helper function: parse content-length body */
bool HttpRequest::parseContentLengthBody(const std::string& body_section)
{
    // 1. validate content length was set
    if (content_length_ < 0)
        return false;

    // 2. handle zero-length body
    if (content_length_ == 0)
    {
        if (body_section.length() > 0)
            return false;
        body_ = "";
        return true;
    }

    // 3. check if have enough data (checked in isRequestComplete)
    // 4. check if have too much data (checked in isRequestComplete)

    // 5. parse the exact amount of body data
    body_ = body_section.substr(0, content_length_);

    // 6. validate body size limits
    if (body_.length() > MAX_BODY_SIZE)
        return false;
        
    return true;
}

/* parse the body 
    - return true if parsed successfully, false otherwise
    - body stored in std::string body_
*/
bool HttpRequest::parseBody(const std::string& body_section)
{
// validate header non-conflicting
    if (chunked_encoding_ && content_length_ >= 0)
        return false;

// check if the method can have body
    // for GET & DELETE
    if (!methodCanHaveBody(method_str_))
    {
    //     if (chunked_encoding_ || content_length_ >= 0)
    //         return false; // GET, DELETE shouldn't have body
        body_ = "";
        return true;
    }

// enforce size limit early
    if (content_length_ > static_cast<long>(MAX_BODY_SIZE))
        return false; // content exceed max size limit
    
// for POST: parse body based on type
    // if transfer-encoding
    if (chunked_encoding_)
    {
        return decodeChunkedBody(body_section);
    }
    // if content-length
    else if (content_length_ >= 0)
    {
        return parseContentLengthBody(body_section);
    }
    // POST without content-length - allow parsing, validate later
    else
    {
        body_ = "";
        return true;
    }
}

/* parse complete request */
bool HttpRequest::parseRequest(const std::string& complete_request)
{
    // pre-validation: check if the request is empty or too large
    if (complete_request.empty() || complete_request.length() > MAX_REQUEST_SIZE)
        return false;
    // check completeness
    if (isRequestComplete(complete_request) != REQUEST_COMPLETE)
        return false; // not complete, not ready for parsing
    
    // split the request into components
    size_t first_crlf = complete_request.find("\r\n");
    size_t header_end = complete_request.find("\r\n\r\n");
    if (first_crlf == std::string::npos || header_end == std::string::npos)
        return false;

    // extract components
    std::string request_line = complete_request.substr(0, first_crlf);
    std::string header_section;
    size_t header_start = first_crlf + 2;
    size_t header_length = header_end - header_start;
    if (header_length < 0)
        header_section = "";
    else
        header_section = complete_request.substr(header_start, header_length);
    std::string body_section;
    size_t body_start = header_end + 4;
    if (body_start >= complete_request.length())
        body_section = "";
    else
        body_section = complete_request.substr(body_start);

    // parse componenets
    if (!parseRequestLine(request_line)
        || !parseHeaders(header_section)
        || !parseBody(body_section))
        return false;

    // set successful parsing flag
    is_parsed_ = true;
    return true;
}

// ============================================================================
// Validation                                                  
// ============================================================================

/* basic input check, prepare for further validation */
ValidationResult HttpRequest::inputValidation() const
{
    // check if request is complete
    if (!is_complete_)
        return BAD_REQUEST;
    // check if request is parsed
    if (!is_parsed_)
        return BAD_REQUEST;
    // check if essential components exist
    if (method_str_.empty() || uri_.empty() || http_version_.empty())
        return INVALID_REQUEST_LINE;
    return VALID_REQUEST; // for further validation
}

/* validate URI in request line */
ValidationResult HttpRequest::validateURI() const
{
    // basic check: should start with '/'
    if (uri_[0] != '/')
        return INVALID_URI;
    // check for boundary
    if (uri_.length() > MAX_URI_LENGTH)
        return URI_TOO_LONG;
    // path traversal security check
    if (uri_.find("../") != std::string::npos
        || uri_.find("..%2f") != std::string::npos
        || uri_.find("..%2F") != std::string::npos
        || uri_.find("%2e%2e/") != std::string::npos
        || uri_.find("%2e%2e%2f") != std::string::npos
        || uri_.find("%2E%2E/") != std::string::npos
        || uri_.find("%2E%2E%2F") != std::string::npos
    )
        return INVALID_URI;
    // check for null bytes and control char
    for (size_t i = 0; i < uri_.length(); i++)
    {
        char c = uri_[i];
        if (c >= 0 && c <= 31) // control characters
            return INVALID_URI;
        if (c == 127) // DEL
            return INVALID_URI;
    }
    // validate allowed char
    for (size_t i = 0; i < uri_.length(); i++)
    {
        char c = uri_[i];
        // RFC 3968 uri char 
        if (!(isalnum(c) || 
                c == '-' || c == '_' || c == '.' || c == '~' || // unreserved
                c == '/' || // path separator
                c == '%' || // pecent-encoding
                c == '?' || c == '=' || c == '&' || c == '+')) // query string
            return INVALID_URI;
    }
    // validate percent-encoding format
    for (size_t i = 0; i < uri_.length(); i++)
    {
        if (uri_[i] == '%')
        {
            // must have 2 hex digits after %
            if (i + 2 >= uri_.length())
                return INVALID_URI;
            char hex1 = uri_[i + 1];
            char hex2 = uri_[i + 2];
            if (!isxdigit(hex1) || !isxdigit(hex2))
                return INVALID_URI;
            i += 2; // skip the 2 hex digits
        }
    }
    return VALID_REQUEST;
}

ValidationResult HttpRequest::validateRequestLine() const
{
    // validate method
    if (!isValidMethod(method_str_))
        return INVALID_METHOD;
    // validate URI
    ValidationResult uri_result = validateURI();
    if (uri_result != VALID_REQUEST)
        return uri_result;
    // validate HTTP version
    if (http_version_ != "HTTP/1.1")
        return INVALID_HTTP_VERSION;
    return VALID_REQUEST;
}

/* helper function: count the appearance of header_name in the headers_
    @return: the count of header_name in headers_
*/
static size_t headerCount(const std::multimap<std::string, std::string>& headers, std::string header_name)
{
    return headers.count(header_name);
}

/* helper function: header name format validation
    @format: header name can only contain tchar (RFC 7230)
        tchar = "!" / "#" / "$" / "%" / "&" / "'" / "*" / "+" / "-" / "." / "^" / "_" / "`" / "|" / "~" / DIGIT / ALPHA
        DIGIT = 0-9
        ALPHA = a-zA-Z
    @return: true if valid, false otherwise
*/
static bool headerNameValid(std::string& name)
{
    for (size_t i = 0; i < name.length(); i++)
    {
        char c = name[i];
        // RFC 7230 tchar
        if (!(isalnum(c) || 
                c == '!' || c == '#' || c == '$' || c == '%' || 
                c == '%' || c == '&' || c == '\'' || c == '*' || 
                c == '+' || c == '-' || c == '.' || c == '^' || 
                c == '_' || c == '`' || c == '|' || c == '~'))
            return false;
    }
    return true;
}

/* helper function: header value format validation
    @format: 
    allowed
        - visible ASCII (33-126)
        - obs-text (128-255) for backward compatibility
        - spaces, tabs (32, 9) only allowed between visible chars
    prohibited
        - control characters (0-31, 127)
        - no leading or trailing spaces/tabs
        - no line breaks
    @return: true if valid, false otherwise
*/
static bool headerValueValid(std::string& value)
{
    for (size_t i = 0; i < value.length(); i++)
    {
        unsigned char c = value[i];
        if (! (c >= 32 || c == 9 || c >= 128))
            return false;
    }
    return true;
}

ValidationResult HttpRequest::validateHeader() const
{
// validate host header
    // only one host header is allowed
    if (headerCount(headers_, "host") != 1)
        return INVALID_HEADER;
    // host value must be present
    std::string host_value = getHost();
    if (host_value.empty())
        return INVALID_HEADER;
    // TBU: host value format check

// transfer-encoding validation
    // only one transfer-encoding header if present
    if (headerCount(headers_, "transfer-encoding") > 1)
        return INVALID_HEADER;
    // if transfer-encoding is present, must be chunked
    std::multimap<std::string, std::string>::const_iterator te_it = headers_.find("transfer-encoding");
    if (te_it != headers_.end())
    {
        std::string te_value = te_it->second;
        std::transform(te_value.begin(), te_value.end(), te_value.begin(), ::tolower);
        if (te_value.find("chunked") == std::string::npos)
            return INVALID_HEADER; // only support chunked encoding
    }

// content-length validation
    // only one content-length header if present
    if (headerCount(headers_, "content-length") > 1)
        return INVALID_HEADER;
    // content-length value must be valid if present
    if (!methodCanHaveBody(method_str_) && (content_length_ > 0 || chunked_encoding_))
        return METHOD_BODY_MISMATCH; // GET, DELETE shouldn't have body
    if (methodCanHaveBody(method_str_) && !chunked_encoding_ && content_length_ < 0)
        return LENGTH_REQUIRED; // POST must have content-length if chunked not set

// header format issues
    // header name & value format
    for (std::multimap<std::string, std::string>::const_iterator it = headers_.begin(); it != headers_.end(); ++it)
    {
        std::string name = it->first;
        if (!headerNameValid(name))
            return INVALID_HEADER;
        std::string value = it->second;
        if (!headerValueValid(value))
            return INVALID_HEADER;
    }

    return VALID_REQUEST;
}

ValidationResult HttpRequest::validateBody() const
{
// content-length vs actual body size
    if (content_length_ >= 0 && body_.length() != static_cast<size_t>(content_length_))
        return BAD_REQUEST; // content-length mismatch
    
/* other checks have been done before
    - body size limit
    - method-body mismatch
    - conflicting headers
    - chunked encoding validation
*/ 
    return VALID_REQUEST;
}

ValidationResult HttpRequest::validateRequest()
{
    // 1. input validation
    ValidationResult input_result = inputValidation();
    if (input_result != VALID_REQUEST)
    {
        validation_status_ = input_result;
        return input_result;
    }
    // 2. validate request line
    ValidationResult line_result = validateRequestLine();
    if (line_result != VALID_REQUEST)
    {
        validation_status_ = line_result;
        return line_result;
    }
    // 3. validate headers
    ValidationResult header_result = validateHeader();
    if (header_result != VALID_REQUEST)
    {
        validation_status_ = header_result;
        return header_result;
    }
    // 4. validate body TBU
    ValidationResult body_result = validateBody();
    if (body_result != VALID_REQUEST)
    {
        validation_status_ = body_result;
        return body_result;
    }
    // if all validations pass
    validation_status_ = VALID_REQUEST;
    return VALID_REQUEST;
}

// ============================================================================
// Getters                                                  
// ============================================================================

const std::string& HttpRequest::getMethodStr() const {
    return method_str_;
}

const std::string& HttpRequest::getURI() const {
    return uri_;
}

const std::string& HttpRequest::getQueryString() const {
    return query_string_;
}

const std::string& HttpRequest::getHttpVersion() const {
    return http_version_;
}

const std::string& HttpRequest::getBody() const {
    return body_;
}

bool HttpRequest::getIsComplete() const
{
    return is_complete_;
}

bool HttpRequest::getIsParsed() const
{
    return is_parsed_;
}

// return the host value from the header, empty string if not found
std::string HttpRequest::getHost() const
{
    std::multimap<std::string, std::string>::const_iterator it = headers_.find("host");
    if (it != headers_.end())
        return it->second;
    return "";
}

// return the connection status
bool HttpRequest::getConnection() const
{
    return keep_alive_;
}

// return the validation request result
ValidationResult HttpRequest::getValidationStatus() const
{
    return validation_status_;
}

// ============================================================================
// Setters
// ============================================================================

void HttpRequest::setConnection(bool status)
{
    keep_alive_ = status;
}

void HttpRequest::setValidationResult(ValidationResult result)
{
    validation_status_ = result;
}