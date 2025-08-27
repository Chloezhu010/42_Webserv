#ifndef HTTP_REQUEST_HPP
#define HTTP_REQUEST_HPP

#include <string>
#include <map>
#include <vector>
#include <algorithm>
#include <sstream>

enum RequestStatus {
    INCOMPLETE, //0
    COMPLETE, //1
    OVERSIZED, //2
    INVALID, //3
};

class HttpRequest {
private:
    std::string method;
    std::string uri;
    std::string http_version;
    std::map<std::string, std::string> headers;
    std::string body;
    bool is_valid;

public:
    // HttpRequest();
    // ~HttpRequest();

    // check request completeness before parsing
    RequestStatus isRequestComplete(const std::string& request_buffer) const;
    
    // main parsing method
    void parse(const std::string& request);
    
    // getters
    const std::string& getMethod() const;
    const std::string& getUri() const;
    const std::string& getHttpVersion() const;
    
    // validation methods (after parsing)
    bool requireBody() const;
    size_t getContentLength() const;
    bool isValid() const;

private:
    /* extract & validate method */
    std::string extractMethod(const std::string& request_buffer) const;
    bool isValidMethod(const std::string& method) const;
    
    /* extract & validate http version */
    int extractHTTPVersion(const std::string& header_section) const;
    bool isValidVersion(const int version) const;

    /* extract & validate host */
    std::string extractHost(const std::string& header_section) const;

    /* validation of header section */

    /* extract content length */
    bool methodCanHaveBody(const std::string& method) const;
    long extractContentLength(const std::string& header_section) const;
    
    /* extract & validate transfer-encoding */

    
};

#endif