#ifndef HTTP_REQUEST_HPP
#define HTTP_REQUEST_HPP

#include <iostream>
#include <string>
#include <map>
#include <vector>
#include <algorithm>
#include <sstream>
#include <utility>

enum RequestStatus {
    NEED_MORE_DATA,
    REQUEST_COMPLETE,
    REQUEST_TOO_LARGE,
    INVALID_REQUEST
};

enum ValidationResult {
    // processing stsate
    NOT_VALIDATED,
    NEED_MORE_DATA,         // still reading request
    // success responses (2xx)
    VALID_REQUEST,          // 200
    CREATED,                // 201 created (file uploads, POST operations)
    NO_CONTENT,             // 204 no content (successful DELETE)
    // redirection (3xx)
    MOVED_PERMANENTLY,      // 301 move permanently
    FOUND,                  // 302 found (temporary redirect)
    // client error responses (4xx)
    INVALID_REQUEST_LINE,   // 400 bad request - bad request line
    INVALID_HTTP_VERSION,   // 400 bad request - bad http version
    INVALID_URI,            // 400 bad request - bad uri
    MISSING_HOST_HEADER,    // 400 bad request - missing host header
    INVALID_CONTENT_LENGTH, // 400 bad request - invalid content-length
    CONFLICTING_HEADER,     // 400 bad request - conflicting headers
    METHOD_BODY_MISMATCH,   // 400 bad request - body with GET/DELETE
    INVALID_HEADERS,        // 400 bad request - header format error

    UNAUTORIZED,            // 401 unauthorized (if auth implemented)
    FORBIDDEN,              // 403 forbidden - access denied
    NOT_FOUND,              // 404 not found - resource doesn't exit
    INVALID_METHOD,         // 405 method not allowed
    REQUEST_TIMEOUT,        // 408 request timeout
    CONFLICT,               // 409 conflict - resource conflict
    LENGTH_REQUIRED,        // 411 length required - POST without content-length
    PAYLOAD_TOO_LARGE,      // 413 payload too large - body/ request too large
    URI_TOO_LONG,           // 414 uri too long
    UNSUPPORTED_MEDIA_TYPE, // 415 unsupported media type
    HEADER_TOO_LARGE,       // 431 request header fields too large
    // server error responses (5xx)
    INTERNAL_SERVER_ERROR,  // 500 internal server error
    NOT_IMPLEMENTED,        // 501 not implemented
    BAD_GATEWAY,            // 502 bad gateway - CGI errors
    SERVICE_UNAVAILABLE,    // 503 Service unavailable - temp overload
    GATEWAY_TIMEOUT,        // 504 gateway timeout - CGI timeout
    HTTP_VERSION_NOT_SUPPORTED, // 505 http version not supported    
};

// constants (TBD)
const size_t MAX_REQUEST_SIZE = 8*1024*1024;
const size_t MAX_HEADER_SIZE =8*1024;
const size_t MAX_HEADER_COUNT = 100; 
const size_t MAX_URI_LENGTH = 2048;
const size_t MAX_BODY_SIZE = 10*1024*1024;

// ============================================================================
// HTTP REQUEST CLASS                                                                                  
// ============================================================================

class HttpRequest {
private:
    // raw data
    std::string raw_request_;
    std::string request_buffer_;

    // parsed components
    std::string method_str_;
    std::string full_uri_;
    std::string uri_;
    std::string query_string_;
    std::string http_version_;
    std::map<std::string, std::string> headers_;
    std::string body_;

    // metadata
    bool is_complete_;
    bool is_valid_;
    ValidationResult validation_error_;
    long content_length_;
    bool chunked_encoding_;

public:
    // ============================================================================
    // Constructors & Destructors                                                  
    // ============================================================================
    
    HttpRequest();
    ~HttpRequest();

    // ============================================================================
    // Phase 1 Completeness check                                                  
    // ============================================================================
    
    // check request completeness before parsing
    RequestStatus isRequestComplete(const std::string& request_buffer);
    RequestStatus isChunkedBodyComplete(const std::string& request_buffer, size_t header_end) const;
    RequestStatus isContentLengthBodyComplete(const std::string& request_buffer, size_t header_end, long content_length) const;
    
    // data accumulation
    RequestStatus addData(const std::string& new_data);
    void clearBuffer();

    // ============================================================================
    // Phase 2 Parsing                                                  
    // ============================================================================
    
    // parse complete request
    bool parseRequest(const std::string& complete_request);

    // component parsing
    bool parseRequestLine(const std::string& request_line);
    bool parseHeaders(const std::string& header_section);
    bool parseBody(const std::string& body_section);
    
    // parsing help function
    bool decodeChunkedBody(const std::string& body_section);
    bool parseContentLengthBody(const std::string& body_section);

    // ============================================================================
    // Phase 3 Validation                                                  
    // ============================================================================
    
    // main validation entry point
    ValidationResult validationRequest() const;

    // component validation
    ValidationResult validateRequestLine() const;
    ValidationResult validateHeader() const;
    ValidationResult validateBody() const;
    ValidationResult validateURI() const;
    ValidationResult validateHttpVersion() const;

    // ============================================================================
    // Extraction methods                                                  
    // ============================================================================
    
    // method
    std::string extractMethod(const std::string& request_buffer) const;
    bool isValidMethod(const std::string& method) const;

    // url
    std::string extractURI(const std::string& request_line) const;
    std::string extractQueryString(const std::string& uri) const;

    // http version
    std::string extractHTTPVersion(const std::string& request_line) const;

    // header
    long extractContentLength(const std::string& header_section) const;

    // body
    bool methodCanHaveBody(const std::string& method) const;
    bool isChunkedEncoding() const;

    // ============================================================================
    // Getters                                                  
    // ============================================================================
    
    // request components
    // const std::string& getMethod() const;
    const std::string& getMethodStr() const;
    const std::string& getURI() const;
    const std::string& getQueryString() const;
    const std::string& getHttpVersion() const;
    const std::string& getBody() const;
    
    // metadata (TBU)

    // specific headers
    std::string getHost() const;
    std::string getUserAgent() const;
    std::string getContentType() const;
    std::string getConnection() const;

    // ============================================================================
    // Setters                                            
    // ============================================================================

    // ============================================================================
    // Helpers                                        
    // ============================================================================

    

};

#endif