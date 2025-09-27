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

std::string extractBoundary(const std::string& content_type)
{
    // convert to lower case for case-insensitive search
    std::string lowercase_ct = content_type;
    std::transform(lowercase_ct.begin(), lowercase_ct.end(), lowercase_ct.begin(), ::tolower);

    // find "boundary"
    size_t boundary_pos = lowercase_ct.find("boundary");
    if (boundary_pos == std::string::npos)
        return ""; // not found

    // find the "=" after boundary
    size_t equal_pos = lowercase_ct.find('=', boundary_pos + 8); // 8 is length of "boundary"
    if (equal_pos == std::string::npos)
        return ""; // not found

    // extract value after "=" from original string
    size_t value_start = equal_pos + 1; // skip the '='
    size_t value_end = content_type.find(';', value_start);
    if (value_end == std::string::npos)
        value_end = content_type.length(); // till end if no ';'
    std::string boundary = content_type.substr(value_start, value_end - value_start);

    // trim space
    size_t trim_start = boundary.find_first_not_of(" \t\r\n");
    size_t trim_end = boundary.find_last_not_of(" \t\r\n");
    if (trim_start != std::string::npos && trim_end != std::string::npos)
        boundary = boundary.substr(trim_start, trim_end - trim_start + 1);
    
    return boundary;
}

static std::string extractQuoteValue(const std::string& headers, const std::string& key)
{
    size_t pos = headers.find(key);
    if (pos == std::string::npos)
        return "";
    pos += key.length();
    // pos already at the end
    if (pos >= headers.length())
        return "";
    // if the value start with "
    if (headers[pos] == '"'){
        // skip the opening quote
        pos++;
        size_t end = headers.find('"', pos);
        if (end == std::string::npos)
            return "";
        return headers.substr(pos, end - pos);
    } else // unquoted value
    {
        size_t end = pos;
        // find end of the value (stops at semicolon, space, or end of string)
        while (end < headers.length()
                && headers[end] != ';' && headers[end] != ' ' && headers[end] != '\t'
                && headers[end] != '\r' && headers[end] != '\n')
            end++;
        return headers.substr(pos, end - pos);
    }
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
    
    // time_t now = time(0);
    // struct tm* gmtTime = gmtime(&now);

    // std::cout << "raw time: " << now << std::endl;
    // std::cout << "local time: " << ctime(&now);
    // std::cout << "raw gmt time: " << gmtime(&now) << std::endl;
    // std::cout << "readable gmt time: " << asctime(gmtTime);
    // char httpbuffer[100];
    // strftime(httpbuffer, sizeof(httpbuffer), "%a, %d %b %Y %H:%M:%S GMT", gmtTime);
    // std::cout << "http format time: " << httpbuffer << std::endl;
    
    // std::string content_type1 = "multipart/form-data; boundary=----WebKitFormBoundary7MA4YWxkTrZu0gW";
    // std::string content_type2 = "multipart/form-data; boundary = ----WebKitFormBoundary7MA4YWxkTrZu0gW ";
    // std::string content_type3 = "multipart/form-data; charset=utf-8";
    // std::string content_type4 = "";
    // std::string content_type5 = "multipart/form-data; boundary=";

    // std::cout << "boundary1: [" << extractBoundary(content_type1) << "]" << std::endl;
    // std::cout << "boundary2: [" << extractBoundary(content_type2) << "]" << std::endl;
    // std::cout << "boundary3: [" << extractBoundary(content_type3) << "]" << std::endl;
    // std::cout << "boundary4: [" << extractBoundary(content_type4) << "]" << std::endl;
    // std::cout << "boundary5: [" << extractBoundary(content_type5) << "]" << std::endl;

    std::string headers1 = "Content-Disposition: form-data; name=\"photo\"; filename=\"vacation.jpg\"";
    std::string headers2 = "Content-Disposition: form-data; name=photo; filename=vacation.jpg";
    std::string headers3 = "Content-Disposition: form-data; name=\" photo\"; filename =\"vacation.jpg\"";

    std::cout << extractQuoteValue(headers1, "name=") << std::endl;
    std::cout << extractQuoteValue(headers1, "filename=") << std::endl;
    std::cout << extractQuoteValue(headers2, "name=") << std::endl;
    std::cout << extractQuoteValue(headers2, "filename=") << std::endl;
    std::cout << extractQuoteValue(headers3, "name=") << std::endl;
    std::cout << extractQuoteValue(headers3, "filename=") << std::endl;
    return 0;
}