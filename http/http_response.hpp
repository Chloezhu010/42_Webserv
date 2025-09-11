#ifndef HTTP_RESPONSE_HPP
#define HTTP_RESPONSE_HPP

#include <iostream>
#include <string>
#include <map>
#include <algorithm>
#include "http_request.hpp"

class HttpResponse
{
private:
    int status_code_;
    std::string status_line_;
    std::map<std::string, std::string> headers_;
    std::string body_;    
public:
    // response status line
    void resultToStatusCode(ValidationResult result);
    std::string getReasonPhase();
    std::string responseStatusLine();

    // response header
    void setHeader(const std::string& name, const std::string& value);
    void setRequiredHeader(const HttpRequest& request);
    std::string responseHeader();

    // response body (TBU)

    // build whole response
    std::string buildFullResponse(const HttpRequest& request);

    // getters
    int getStatusCode() const;

    // setters
    void setBody(std::string body_info);

};

#endif // HTTP_RESPONSE_HPP