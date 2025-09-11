#include "http_response.hpp"

// ============================================================================
// Response status line
// ============================================================================  

/* convert the validation result into int, stored in status_code_ */
void HttpResponse::resultToStatusCode(ValidationResult result)
{
    switch (result)
    {
        // 200 series
        case VALID_REQUEST: status_code_ = 200; break;
        case CREATED: status_code_ = 201; break;
        case NO_CONTENT: status_code_ = 204; break;
        // 300 series
        case MOVED_PERMANENTLY: status_code_ = 301; break;
        case FOUND: status_code_ = 302; break;
        // 400 series
        case BAD_REQUEST:
        case INVALID_REQUEST_LINE:
        case INVALID_HTTP_VERSION:
        case INVALID_URI:
        case INVALID_HEADER:
        case INVALID_CONTENT_LENGTH:
        case CONFLICTING_HEADER:
        case METHOD_BODY_MISMATCH:
        case MISSING_HOST_HEADER:
            status_code_ = 400; break;
        case UNAUTORIZED: status_code_ = 401; break;
        case FORBIDDEN: status_code_ = 403; break;
        case NOT_FOUND: status_code_ = 404; break;
        case INVALID_METHOD: status_code_ = 405; break;
        case REQUEST_TIMEOUT: status_code_ = 408; break;
        case CONFLICT: status_code_ = 409; break;
        case LENGTH_REQUIRED: status_code_ = 411; break;
        case PAYLOAD_TOO_LARGE: status_code_ = 413; break;
        case URI_TOO_LONG: status_code_ = 414; break;
        case UNSUPPORTED_MEDIA_TYPE: status_code_ = 415; break;
        case HEADER_TOO_LARGE: status_code_ = 431; break;
        // 500 series
        case INTERNAL_SERVER_ERROR: status_code_ = 500; break;
        case NOT_IMPLEMENTED: status_code_ = 501; break;
        case BAD_GATEWAY: status_code_ = 502; break;
        case SERVICE_UNAVAILABLE: status_code_ = 503; break;
        case GATEWAY_TIMEOUT: status_code_ = 504; break;
        default: status_code_ = 500; break; // default to 500 internal server error
    }
}

/* return the string of reason phase based on the status code */
std::string HttpResponse::getReasonPhase()
{
    switch (status_code_)
    {
        // 200 series
        case 200: return "OK";
        case 201: return "Created";
        case 204: return "No Content";
        // 300 series
        case 301: return "Moved Permanently";
        case 302: return "Found";
        // 400 series
        case 400: return "Bad Request";
        case 401: return "Unauthorized";
        case 403: return "Forbidden";
        case 404: return "Not Found";
        case 405: return "Method Not Allowed";
        case 408: return "Request Timeout";
        case 409: return "Conflict";
        case 411: return "Length Required";
        case 413: return "Payload Too Large";
        case 414: return "URI Too Long";
        case 415: return "Unsupported Media Type";
        case 431: return "Request Header Fields Too Large";
        // 500 series
        case 500: return "Internal Server Error";
        case 501: return "Not Implemented";
        case 502: return "Bad Gateway";
        case 503: return "Service Unavailable";
        case 504: return "Gateway Timeout";
        default: return "Internal Server Error";
    }
}

/* compose the complete status line for http response
    - return: HTTP/1.1 status_code_ reason_phase \r\n
*/
std::string HttpResponse::responseStatusLine()
{
    std::string http_version = "HTTP/1.1";
    std::string status_code_str = std::to_string(status_code_);
    std::string reason_phase = getReasonPhase();
    
    return (http_version + " " + status_code_str + " " + reason_phase + "\r\n");
}

// match the respose header name with value
void HttpResponse::setHeader(const std::string& name, const std::string& value)
{
    headers_[name] = value;
}

// helper function: GMT time info for http response
static std::string getCurrentDate()
{
    time_t now = time(0);
    struct tm* gmtTime = gmtime(&now);
    // manual formating for http response
    char httpbuffer[100];
    strftime(httpbuffer, sizeof(httpbuffer), "%a, %d %b %Y %H:%M:%S GMT", gmtTime);
    return (std::string(httpbuffer));
}

// set the required response header
void HttpResponse::setRequiredHeader(const HttpRequest& request)
{
    setHeader("Date", getCurrentDate());
    // setHeader("Content-Type", content_type); TBU
    setHeader("Content-Length", std::to_string(body_.length()));
    setHeader("Server", "42_webserv");
    
    bool keep_alive = request.getConnection();
    if (keep_alive)
        setHeader("Connection", "keep-alive");
    else
        setHeader("Connection", "close");
}

/* compose the required header section string in specific order
    - server
    - date
    - content-type: TBU
    - content-length
    - connection
*/
std::string HttpResponse::responseHeader()
{
    std::string header_string;
    std::string header_order[] = {"Server", "Date", "Content-Type", "Content-Length", "Connection"};
    int order_size = 5;
    for (int i = 0; i < order_size; i++)
    {
        std::map<std::string, std::string>::const_iterator it = headers_.find(header_order[i]);
        if (it != headers_.end())
            header_string += it->first + ": " + it->second + "\r\n";
    }
    return header_string;
}

/* build the whole http response */
std::string HttpResponse::buildFullResponse(const HttpRequest& request)
{
    // update status_code_
    resultToStatusCode(request.getValidationStatus());
    // build response status line
    std::string status_line = responseStatusLine();
    // build response header section
    setRequiredHeader(request);
    std::string header_section = responseHeader();
    // hardcoded body section TBU
    setBody("test_test_TBU");
    return (status_line + header_section + "\r\n" + body_);
}


// ============================================================================
// Getters                                               
// ============================================================================  

int HttpResponse::getStatusCode() const
{
    return status_code_;
}

// ============================================================================
// Setters
// ============================================================================ 

void HttpResponse::setBody(std::string body_info)
{
    body_ = body_info;
}