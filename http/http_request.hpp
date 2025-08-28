#ifndef HTTP_REQUEST_HPP
#define HTTP_REQUEST_HPP

#include <string>
#include <map>
#include <vector>
#include <algorithm>
#include <sstream>

// enum RequestStatus {
//     INCOMPLETE, //0
//     COMPLETE, //1
//     OVERSIZED, //2
//     INVALID, //3
// };
enum RequestStatus {
    NEED_MORE_DATA,
    REQUEST_COMPLETE,
    REQUEST_TOO_LARGE,
    INVALID_REQUEST
};

enum ValidationResult {
    VALID_REQUEST,
    INVALID_METHOD, // 405
    INVALID_REQUEST_LINE, // 400
    INVALID_HTTP_VERSION, // 505
    INVALID_URI, // 400
    MISSING_HOST_HEADER, // 400
    INVALID_CONTENT_LENGTH, // 400
    CONFLICTING_HEADER, // 400
    METHOD_BODY_MISMATCH, // 400
    HEADER_TOO_LARGE, // 431
    URI_TOO_LONG, // 414
};

// constants (TBD)
const size_t MAX_REQUEST_SIZE = 8*1024*1024;
const size_t MAX_HEADER_SIZE =8*1024;
const size_t MAX_HEADER = 100;
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
    RequestStatus isRequestComplete(const std::string& request_buffer) const;
    
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
    bool parseBody();
    
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
    const std::string& getMethod() const;
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

    
};

#endif