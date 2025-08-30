#include <iostream>
#include <string>
#include <sstream>
#include <vector>

/* helper function: split the request line by space
    - return the number of parts, split by space
*/
static std::vector<std::string> split_by_space(const std::string& str)
{
    // vector container to store the results
    std::vector<std::string> results;
    // if the string is empty, return empty vector
    if (str.empty())
        return results;
    std::istringstream iss(str);
    std::string token;
    while (iss >> token) {
        results.push_back(token);
    }
    return results;
}

/* parse the request line
    - request line format: METHOD SP URI SP HTTP/VERSION CRLF
    - return true if parsed successfully, false otherwise
*/
bool parseRequestLine(const std::string& request_line)
{
    // cleanup trailing \r\n if any
    std::string clean_line = request_line;
    while (!clean_line.empty()
                && (clean_line.back() == '\r' || clean_line.back() == '\n')) {
        clean_line.pop_back();
    }
    // check for leading/ trailing space or empty line
    if (clean_line.empty() || clean_line.front() == ' ' || clean_line.back() == ' ')
        return false;

    // split the request line by space
    std::vector<std::string> parts = split_by_space(clean_line);
    if (parts.size() != 3)
        return false; // must be exactly 3 parts
    
    // check for empty parts
    for (size_t i = 0; i < parts.size(); i++) {
        if (parts[i].empty())
            return false; // empty part found
    }
      
    // extract method
    std::string method_str_ = parts[0];
    std::cout << "Method: " << method_str_ << std::endl;

    // extract url and query string
    std::string full_uri_ = parts[1];
    size_t query_pos = full_uri_.find('?');
    std::string uri_;
    std::string query_string_;
    if (query_pos != std::string::npos) {
        uri_ = full_uri_.substr(0, query_pos);
        query_string_ = full_uri_.substr(query_pos + 1);
    } else {
        uri_ = full_uri_;
        query_string_ = "";
    }
    std::cout << "Full URI: " << full_uri_ << std::endl;
    std::cout << "URI: " << uri_ << std::endl;
    std::cout << "Query String: " << query_string_ << std::endl;

    // extract http version
    std::string http_version_ = parts[2];
    std::cout << "HTTP Version: " << http_version_ << std::endl;

    return true;
}

/* request line parsing test */
int main()
{
    // correct request line
    std::string test1 = "GET /about HTTP/1.1";
    std::string test2 = "GET /about HTTP/1.1\r";
    // acceptable
    std::string test3 = "GET    /about?name=chloe   HTTP/1.1";
    // incorrect request line
    std::string test4 = "GET / ";
    std::string test5 = "GET";
    std::string test6 = "";
    std::string test7 = "GET / HTTP/1.1 EXTRA";
    std::string test8 = "GET    HTTP/1.1  ";
    std::string test9 = " / HTTP/1.1 ";
    std::string test10 = "   GET / HTTP/1.1";

    // std::vector<std::string> res1 = split_by_space(test1);
    // std::vector<std::string> res2 = split_by_space(test2);
    // std::vector<std::string> res3 = split_by_space(test3);
    // std::vector<std::string> res4 = split_by_space(test4);
    // std::vector<std::string> res5 = split_by_space(test5);
    // std::vector<std::string> res6 = split_by_space(test6);
    // std::vector<std::string> res7 = split_by_space(test7);
    // std::vector<std::string> res8 = split_by_space(test8);
    // std::vector<std::string> res9 = split_by_space(test9);
    // std::vector<std::string> res10 = split_by_space(test10);

    // std::cout << "Test 1 parts: " << res1.size() << std::endl;
    // std::cout << "Test 2 parts: " << res2.size() << std::endl;
    // std::cout << "Test 3 parts: " << res3.size() << std::endl;
    // std::cout << "Test 4 parts: " << res4.size() << std::endl;
    // std::cout << "Test 5 parts: " << res5.size() << std::endl;
    // std::cout << "Test 6 parts: " << res6.size() << std::endl;
    // std::cout << "Test 7 parts: " << res7.size() << std::endl;
    // std::cout << "Test 8 parts: " << res8.size() << std::endl;
    // std::cout << "Test 9 parts: " << res9.size() << std::endl;
    // std::cout << "Test 10 parts: " << res10.size() << std::endl;

    std::cout << "Parsing test 1: " << parseRequestLine(test1) << std::endl;
    std::cout << "Parsing test 2: " << parseRequestLine(test2) << std::endl;
    std::cout << "Parsing test 3: " << parseRequestLine(test3) << std::endl;
    std::cout << "Parsing test 4: " << parseRequestLine(test4) << std::endl;
    std::cout << "Parsing test 5: " << parseRequestLine(test5) << std::endl;
    std::cout << "Parsing test 6: " << parseRequestLine(test6) << std::endl;
    std::cout << "Parsing test 7: " << parseRequestLine(test7) << std::endl;
    std::cout << "Parsing test 8: " << parseRequestLine(test8) << std::endl;
    std::cout << "Parsing test 9: " << parseRequestLine(test9) << std::endl;
    std::cout << "Parsing test 10: " << parseRequestLine(test10) << std::endl;
}