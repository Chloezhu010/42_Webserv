#include <string>
#include <iostream>
#include <algorithm>
#include <sstream>

size_t MAX_BODY_SIZE = 100;
long content_length_ = 11;
std::string body_;

bool parseContentLengthBody(const std::string& body_section)
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

    // 5. extract exact amount of body data
    body_ = body_section.substr(0, content_length_);

    // 6. validate body size limits
    if (body_.length() > MAX_BODY_SIZE)
        return false;
        
    return true;
}


int main()
{
    // std::string body_section = "  hello_world";

    // std::cout << "parsed outcome: " << parseContentLengthBody(body_section) << std::endl;
    // std::cout << "parsed body: " << body_ << std::endl;

    // std::string test = " 12abc";
    // char *endptr;
    // long cl = strtol(test.c_str(), &endptr, 10);
    // long content_length_ = 0;
    // if (*endptr != 0 || cl < 0)
    //     content_length_ = -1;
    // else
    //     content_length_ = cl;
    // std::cout << "content_length_: " << content_length_ << std::endl;
    
    time_t now = time(0);
    struct tm* gmtTime = gmtime(&now);

    std::cout << "raw time: " << now << std::endl;
    std::cout << "local time: " << ctime(&now);
    std::cout << "raw gmt time: " << gmtime(&now) << std::endl;
    std::cout << "readable gmt time: " << asctime(gmtTime);
    char httpbuffer[100];
    strftime(httpbuffer, sizeof(httpbuffer), "%a, %d %b %Y %H:%M:%S GMT", gmtTime);
    std::cout << "http format time: " << httpbuffer << std::endl;
    
    return 0;
}