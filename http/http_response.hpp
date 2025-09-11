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

    // response body


    // getters
    int getStatusCode() const;

};

#endif // HTTP_RESPONSE_HPP