#include <string>
#include <vector>
#include <iostream>
#include <cassert>
#include "../http/http_request.hpp"

// ============================================================================
// TEST DATA STRUCTURE
// ============================================================================

struct ParseRequestTestCase {
    std::string name;
    std::string complete_request;
    bool expected_result;
    // Expected parsed values (only checked if parsing succeeds)
    std::string expected_method;
    std::string expected_uri;
    std::string expected_query_string;
    std::string expected_version;
    std::string expected_body;
    std::string expected_host;
    std::string description;
    
    ParseRequestTestCase(const std::string& n, const std::string& req, bool result,
                        const std::string& method = "", const std::string& uri = "",
                        const std::string& query = "", const std::string& version = "",
                        const std::string& body = "", const std::string& host = "",
                        const std::string& desc = "")
        : name(n), complete_request(req), expected_result(result),
          expected_method(method), expected_uri(uri), expected_query_string(query),
          expected_version(version), expected_body(body), expected_host(host),
          description(desc) {}
};

// ============================================================================
// HELPER FUNCTIONS
// ============================================================================

std::string boolToString(bool value) {
    return value ? "true" : "false";
}

std::string escapeString(const std::string& str) {
    std::string result;
    for (size_t i = 0; i < str.length(); i++) {
        if (str[i] == '\r') result += "\\r";
        else if (str[i] == '\n') result += "\\n";
        else if (str[i] == '\t') result += "\\t";
        else result += str[i];
    }
    return result;
}

// ============================================================================
// TEST CASE DEFINITIONS
// ============================================================================

std::vector<ParseRequestTestCase> createParseRequestTestCases() {
    std::vector<ParseRequestTestCase> test_cases;
    
    // ========================================================================
    // 1. VALID GET REQUESTS
    // ========================================================================
    
    test_cases.push_back(ParseRequestTestCase(
        "simple_get_request",
        "GET /index.html HTTP/1.1\r\nHost: example.com\r\n\r\n",
        true,
        "GET", "/index.html", "", "HTTP/1.1", "", "example.com",
        "Simple GET request should parse correctly"
    ));
    
    test_cases.push_back(ParseRequestTestCase(
        "get_with_query_string",
        "GET /search?q=test&lang=en HTTP/1.1\r\nHost: example.com\r\nUser-Agent: Mozilla/5.0\r\n\r\n",
        true,
        "GET", "/search", "q=test&lang=en", "HTTP/1.1", "", "example.com",
        "GET request with query string should parse correctly"
    ));
    
    test_cases.push_back(ParseRequestTestCase(
        "get_root_path",
        "GET / HTTP/1.1\r\nHost: localhost:8080\r\nConnection: keep-alive\r\n\r\n",
        true,
        "GET", "/", "", "HTTP/1.1", "", "localhost:8080",
        "GET request to root path should parse correctly"
    ));
    
    test_cases.push_back(ParseRequestTestCase(
        "get_with_multiple_headers",
        "GET /api/users HTTP/1.1\r\nHost: api.example.com\r\nUser-Agent: curl/7.68.0\r\nAccept: application/json\r\nAuthorization: Bearer token123\r\n\r\n",
        true,
        "GET", "/api/users", "", "HTTP/1.1", "", "api.example.com",
        "GET request with multiple headers should parse correctly"
    ));
    
    // ========================================================================
    // 2. VALID POST REQUESTS
    // ========================================================================
    
    test_cases.push_back(ParseRequestTestCase(
        "post_with_content_length",
        "POST /submit HTTP/1.1\r\nHost: example.com\r\nContent-Type: application/json\r\nContent-Length: 24\r\n\r\n{\"name\":\"John\",\"age\":30}",
        true,
        "POST", "/submit", "", "HTTP/1.1", "{\"name\":\"John\",\"age\":30}", "example.com",
        "POST request with Content-Length should parse correctly"
    ));
    
    test_cases.push_back(ParseRequestTestCase(
        "post_with_chunked_encoding",
        "POST /upload HTTP/1.1\r\nHost: example.com\r\nTransfer-Encoding: chunked\r\n\r\n7\r\nMozilla\r\n9\r\nDeveloper\r\n0\r\n\r\n",
        true,
        "POST", "/upload", "", "HTTP/1.1", "MozillaDeveloper", "example.com",
        "POST request with chunked encoding should parse correctly"
    ));
    
    test_cases.push_back(ParseRequestTestCase(
        "post_form_data",
        "POST /login HTTP/1.1\r\nHost: example.com\r\nContent-Type: application/x-www-form-urlencoded\r\nContent-Length: 30\r\n\r\nusername=admin&password=secret",
        true,
        "POST", "/login", "", "HTTP/1.1", "username=admin&password=secret", "example.com",
        "POST request with form data should parse correctly"
    ));
    
    test_cases.push_back(ParseRequestTestCase(
        "post_empty_body",
        "POST /ping HTTP/1.1\r\nHost: example.com\r\nContent-Length: 0\r\n\r\n",
        true,
        "POST", "/ping", "", "HTTP/1.1", "", "example.com",
        "POST request with empty body should parse correctly"
    ));
    
    // ========================================================================
    // 3. VALID DELETE REQUESTS
    // ========================================================================
    
    test_cases.push_back(ParseRequestTestCase(
        "delete_request",
        "DELETE /users/123 HTTP/1.1\r\nHost: api.example.com\r\nAuthorization: Bearer token\r\n\r\n",
        true,
        "DELETE", "/users/123", "", "HTTP/1.1", "", "api.example.com",
        "DELETE request should parse correctly"
    ));
    
    test_cases.push_back(ParseRequestTestCase(
        "delete_with_query",
        "DELETE /cache?type=all HTTP/1.1\r\nHost: admin.example.com\r\n\r\n",
        true,
        "DELETE", "/cache", "type=all", "HTTP/1.1", "", "admin.example.com",
        "DELETE request with query string should parse correctly"
    ));
    
    // ========================================================================
    // 4. INVALID REQUESTS - SIZE AND COMPLETENESS
    // ========================================================================
    
    test_cases.push_back(ParseRequestTestCase(
        "empty_request",
        "",
        false,
        "", "", "", "", "", "",
        "Empty request should fail"
    ));
    
    test_cases.push_back(ParseRequestTestCase(
        "request_too_large",
        std::string("GET /") + std::string(MAX_REQUEST_SIZE, 'A') + " HTTP/1.1\r\nHost: example.com\r\n\r\n",
        false,
        "", "", "", "", "", "",
        "Request larger than MAX_REQUEST_SIZE should fail"
    ));
    
    test_cases.push_back(ParseRequestTestCase(
        "incomplete_request_no_headers_end",
        "GET /index.html HTTP/1.1\r\nHost: example.com\r\n",
        false,
        "", "", "", "", "", "",
        "Incomplete request without \\r\\n\\r\\n should fail"
    ));
    
    test_cases.push_back(ParseRequestTestCase(
        "incomplete_post_missing_body",
        "POST /submit HTTP/1.1\r\nHost: example.com\r\nContent-Length: 10\r\n\r\nShort",
        false,
        "", "", "", "", "", "",
        "POST with Content-Length but insufficient body data should fail completeness check"
    ));
    
    // ========================================================================
    // 5. INVALID REQUESTS - MALFORMED REQUEST LINE
    // ========================================================================
    
    test_cases.push_back(ParseRequestTestCase(
        "missing_method",
        " /index.html HTTP/1.1\r\nHost: example.com\r\n\r\n",
        false,
        "", "", "", "", "", "",
        "Request line with leading space should fail"
    ));
    
    test_cases.push_back(ParseRequestTestCase(
        "missing_uri",
        "GET  HTTP/1.1\r\nHost: example.com\r\n\r\n",
        false,
        "", "", "", "", "", "",
        "Request line with empty URI should fail"
    ));
    
    test_cases.push_back(ParseRequestTestCase(
        "missing_http_version",
        "GET /index.html \r\nHost: example.com\r\n\r\n",
        false,
        "", "", "", "", "", "",
        "Request line with trailing space should fail"
    ));
    
    test_cases.push_back(ParseRequestTestCase(
        "too_many_spaces",
        "GET /index.html HTTP/1.1 extra\r\nHost: example.com\r\n\r\n",
        false,
        "", "", "", "", "", "",
        "Request line with extra parts should fail"
    ));
    
    test_cases.push_back(ParseRequestTestCase(
        "empty_request_line",
        "\r\nHost: example.com\r\n\r\n",
        false,
        "", "", "", "", "", "",
        "Empty request line should fail"
    ));
    
    // ========================================================================
    // 6. INVALID REQUESTS - MALFORMED HEADERS
    // ========================================================================
    
    test_cases.push_back(ParseRequestTestCase(
        "missing_host_header",
        "GET /index.html HTTP/1.1\r\nUser-Agent: Test\r\n\r\n",
        false,
        "", "", "", "", "", "",
        "HTTP/1.1 request without Host header should fail"
    ));
    
    test_cases.push_back(ParseRequestTestCase(
        "invalid_header_format",
        "GET /index.html HTTP/1.1\r\nHost example.com\r\n\r\n",
        false,
        "", "", "", "", "", "",
        "Header without colon should fail"
    ));
    
    test_cases.push_back(ParseRequestTestCase(
        "empty_header_name",
        "GET /index.html HTTP/1.1\r\nHost: example.com\r\n: empty-name\r\n\r\n",
        false,
        "", "", "", "", "", "",
        "Header with empty name should fail"
    ));
    
    // ========================================================================
    // 7. INVALID REQUESTS - BODY VIOLATIONS
    // ========================================================================
    
    test_cases.push_back(ParseRequestTestCase(
        "get_with_content_length",
        "GET /index.html HTTP/1.1\r\nHost: example.com\r\nContent-Length: 5\r\n\r\nHello",
        false,
        "", "", "", "", "", "",
        "GET request with Content-Length should fail"
    ));
    
    test_cases.push_back(ParseRequestTestCase(
        "delete_with_body",
        "DELETE /resource HTTP/1.1\r\nHost: example.com\r\nContent-Length: 4\r\n\r\ndata",
        false,
        "", "", "", "", "", "",
        "DELETE request with body should fail"
    ));
    
    test_cases.push_back(ParseRequestTestCase(
        "post_conflicting_headers",
        "POST /upload HTTP/1.1\r\nHost: example.com\r\nTransfer-Encoding: chunked\r\nContent-Length: 10\r\n\r\n5\r\nHello\r\n0\r\n\r\n",
        false,
        "", "", "", "", "", "",
        "POST with both Transfer-Encoding and Content-Length should fail"
    ));
    
    test_cases.push_back(ParseRequestTestCase(
        "post_no_body_indicators",
        "POST /submit HTTP/1.1\r\nHost: example.com\r\n\r\nSome data here",
        false,
        "", "", "", "", "", "",
        "POST without Content-Length or Transfer-Encoding should fail"
    ));
    
    // ========================================================================
    // 8. EDGE CASES
    // ========================================================================
    
    test_cases.push_back(ParseRequestTestCase(
        "minimal_valid_request",
        "GET / HTTP/1.1\r\nHost: a\r\n\r\n",
        true,
        "GET", "/", "", "HTTP/1.1", "", "a",
        "Minimal valid request should parse correctly"
    ));
    
    test_cases.push_back(ParseRequestTestCase(
        "long_uri_valid",
        "GET /" + std::string(1000, 'a') + " HTTP/1.1\r\nHost: example.com\r\n\r\n",
        true,
        "GET", "/" + std::string(1000, 'a'), "", "HTTP/1.1", "", "example.com",
        "Long but valid URI should parse correctly"
    ));
    
    test_cases.push_back(ParseRequestTestCase(
        "uri_too_long",
        "GET /" + std::string(MAX_URI_LENGTH + 1, 'a') + " HTTP/1.1\r\nHost: example.com\r\n\r\n",
        false,
        "", "", "", "", "", "",
        "URI exceeding MAX_URI_LENGTH should fail"
    ));
    
    test_cases.push_back(ParseRequestTestCase(
        "complex_query_string",
        "GET /search?q=hello%20world&filter=type:image&sort=date&page=1 HTTP/1.1\r\nHost: example.com\r\n\r\n",
        true,
        "GET", "/search", "q=hello%20world&filter=type:image&sort=date&page=1", "HTTP/1.1", "", "example.com",
        "Complex query string should parse correctly"
    ));
    
    test_cases.push_back(ParseRequestTestCase(
        "many_headers",
        "GET /api HTTP/1.1\r\nHost: example.com\r\nUser-Agent: Test\r\nAccept: */*\r\nConnection: close\r\nCache-Control: no-cache\r\nAuthorization: Bearer xyz\r\n\r\n",
        true,
        "GET", "/api", "", "HTTP/1.1", "", "example.com",
        "Request with many headers should parse correctly"
    ));
    
    return test_cases;
}

// ============================================================================
// TEST EXECUTION FUNCTIONS
// ============================================================================

bool validateParsedRequest(const HttpRequest& request, const ParseRequestTestCase& test) {
    // Only validate parsed components if parsing was expected to succeed
    if (!test.expected_result) {
        return true; // Don't validate content for failed parsing
    }
    
    bool valid = true;
    
    // Validate method
    if (!test.expected_method.empty() && request.getMethodStr() != test.expected_method) {
        std::cout << "   Method mismatch: expected [" << test.expected_method 
                  << "], got [" << request.getMethodStr() << "]" << std::endl;
        valid = false;
    }
    
    // Validate URI
    if (!test.expected_uri.empty() && request.getURI() != test.expected_uri) {
        std::cout << "   URI mismatch: expected [" << test.expected_uri 
                  << "], got [" << request.getURI() << "]" << std::endl;
        valid = false;
    }
    
    // Validate query string
    if (request.getQueryString() != test.expected_query_string) {
        std::cout << "   Query string mismatch: expected [" << test.expected_query_string 
                  << "], got [" << request.getQueryString() << "]" << std::endl;
        valid = false;
    }
    
    // Validate HTTP version
    if (!test.expected_version.empty() && request.getHttpVersion() != test.expected_version) {
        std::cout << "   HTTP version mismatch: expected [" << test.expected_version 
                  << "], got [" << request.getHttpVersion() << "]" << std::endl;
        valid = false;
    }
    
    // Validate body
    if (request.getBody() != test.expected_body) {
        std::cout << "   Body mismatch: expected [" << test.expected_body 
                  << "], got [" << request.getBody() << "]" << std::endl;
        valid = false;
    }
    
    // // Validate Host header
    // if (!test.expected_host.empty() && request.getHost() != test.expected_host) {
    //     std::cout << "   Host header mismatch: expected [" << test.expected_host 
    //               << "], got [" << request.getHost() << "]" << std::endl;
    //     valid = false;
    // }
    
    return valid;
}

void testParseRequest() {
    std::cout << "\n" << std::string(50, '-') << std::endl;
    std::cout << "TESTING COMPLETE REQUEST PARSING" << std::endl;
    std::cout << std::string(50, '-') << std::endl;
    
    std::vector<ParseRequestTestCase> test_cases = createParseRequestTestCases();
    int passed = 0;
    int total = test_cases.size();
    
    for (size_t i = 0; i < test_cases.size(); i++) {
        const ParseRequestTestCase& test = test_cases[i];
        std::cout << "DEBUG: Starting test " << (i + 1) << ": " << test.name << std::endl;
        std::cout << "DEBUG: " << test.description << std::endl;
        
        try {
            HttpRequest request;
            
            // Test: Parse the complete request
            bool result = request.parseRequest(test.complete_request);
            
            // Check parsing result
            if (result == test.expected_result) {
                // If parsing succeeded, validate parsed components
                if (result) {
                    if (validateParsedRequest(request, test)) {
                        std::cout << "✅ PASS: " << test.name << std::endl;
                        passed++;
                    } else {
                        std::cout << "❌ FAIL: " << test.name << " (parsing succeeded but content validation failed)" << std::endl;
                    }
                } else {
                    std::cout << "✅ PASS: " << test.name << " (correctly failed)" << std::endl;
                    passed++;
                }
            } else {
                std::cout << "❌ FAIL: " << test.name << std::endl;
                std::cout << "   Expected parsing result: " << boolToString(test.expected_result) << std::endl;
                std::cout << "   Got parsing result: " << boolToString(result) << std::endl;
                std::cout << "   Request data: [" << escapeString(test.complete_request) << "]" << std::endl;
            }
        } catch (const std::exception& e) {
            std::cout << "❌ EXCEPTION in test " << test.name << ": " << e.what() << std::endl;
            std::cout << "   Request data was: [" << escapeString(test.complete_request) << "]" << std::endl;
        }
        
        std::cout << std::endl;
    }
    
    std::cout << "\nComplete Request Parsing Results: " << passed << "/" << total << " passed" << std::endl;
}

// ============================================================================
// MAIN FUNCTION
// ============================================================================

int main() {
    std::cout << std::string(60, '=') << std::endl;
    std::cout << "COMPLETE REQUEST PARSING TEST SUITE" << std::endl;
    std::cout << std::string(60, '=') << std::endl;

    testParseRequest();

    return 0;
}