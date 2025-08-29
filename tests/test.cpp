#include <string>
#include <iostream>
#include <algorithm>
#include <sstream>

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
    // if value contains chunked
    return (te_value.find("chunked") != std::string::npos);
}

int main()
{
    std::string header_section = "Host: example.com\r\nTransfer-Encoding: compress\r\n\r\n";

    std::cout << "hasChunkedEncoding: " << hasChunkedEncoding(header_section) << std::endl;

    return 0;
}