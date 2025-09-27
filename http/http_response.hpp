#ifndef HTTP_RESPONSE_HPP
#define HTTP_RESPONSE_HPP

#include <iostream>
#include <string>
#include <map>
#include <sstream>
#include <ctime>
#include <algorithm>
#include "http_request.hpp"

class HttpResponse
{
private:
    int status_code_;
    std::string status_line_;
    std::map<std::string, std::string> headers_;
    std::string body_;
    std::string content_type_;
    
    // Helper methods
    std::string getReasonPhrase() const;
    std::string getCurrentDateGMT() const;
    std::string generateErrorPage(int status_code, const std::string& reason) const;

public:
    // Utility methods
    std::string getContentType(const std::string& file_path) const;
    // Constructor & Destructor
    HttpResponse();
    explicit HttpResponse(int status_code);
    ~HttpResponse();
    
    // Status line methods
    void setStatusCode(int code);
    void resultToStatusCode(ValidationResult result);
    std::string buildStatusLine() const;
    
    // Header methods  
    void setHeader(const std::string& name, const std::string& value);
    void removeHeader(const std::string& name);
    std::string getHeader(const std::string& name) const;
    void setStandardHeaders(const HttpRequest& request);
    void setContentHeaders(const std::string& content, const std::string& file_path = "");
    std::string buildHeaders() const;
    
    // Body methods
    void setBody(const std::string& body);
    void setBodyFromFile(const std::string& file_path);
    void appendBody(const std::string& content);
    void clearBody();
    
    // Response building
    std::string buildFullResponse(const HttpRequest& request);
    std::string buildErrorResponse(int status_code, const std::string& message, HttpRequest& request);
    std::string buildFileResponse(const std::string& file_path, HttpRequest& request);
    
    // Getters
    int getStatusCode() const;
    const std::string& getBody() const;
    size_t getContentLength() const;
    
    // Utility methods
    void reset();
    bool isErrorStatus() const;
    bool isSuccessStatus() const;
};

#endif // HTTP_RESPONSE_HPP