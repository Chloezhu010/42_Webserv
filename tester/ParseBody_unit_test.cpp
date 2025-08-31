#include <string>
#include <vector>
#include <iostream>
#include <cassert>
#include "../http/http_request.hpp"

// ============================================================================
// TEST DATA STRUCTURE
// ============================================================================

struct BodyTestCase {
    std::string name;
    std::string request_data;  // Full request for setup
    std::string body_data;     // Body section to parse
    bool expected_result;
    std::string expected_body; // Expected parsed body content
    std::string description;
    
    BodyTestCase(const std::string& n, const std::string& req, const std::string& body, 
                 bool result, const std::string& expected, const std::string& desc)
        : name(n), request_data(req), body_data(body), expected_result(result), 
          expected_body(expected), description(desc) {}
};

// ============================================================================
// HELPER FUNCTIONS
// ============================================================================

std::string boolToString(bool value) {
    return value ? "true" : "false";
}

// ============================================================================
// TEST CASE DEFINITIONS
// ============================================================================

std::vector<BodyTestCase> createBodyTestCases() {
    std::vector<BodyTestCase> test_cases;
    
    // ========================================================================
    // 1. CONTENT-LENGTH BODY TESTS
    // ========================================================================
    
    test_cases.push_back(BodyTestCase(
        "content_length_simple",
        "POST /submit HTTP/1.1\r\nHost: example.com\r\nContent-Length: 13\r\n\r\n",
        "{\"key\":\"val\"}",
        true,
        "{\"key\":\"val\"}",
        "Simple Content-Length body should parse correctly"
    ));
    
    test_cases.push_back(BodyTestCase(
        "content_length_zero",
        "POST /submit HTTP/1.1\r\nHost: example.com\r\nContent-Length: 0\r\n\r\n",
        "",
        true,
        "",
        "Zero Content-Length should result in empty body"
    ));
    
    test_cases.push_back(BodyTestCase(
        "content_length_mismatch_extra_data",
        "POST /submit HTTP/1.1\r\nHost: example.com\r\nContent-Length: 5\r\n\r\n",
        "Hello World Extra",
        true,
        "Hello",
        "Content-Length should limit body to exact size"
    ));
    
    test_cases.push_back(BodyTestCase(
        "content_length_json",
        "POST /api HTTP/1.1\r\nHost: example.com\r\nContent-Length: 27\r\n\r\n",
        "{\"name\":\"John\",\"age\":30}",
        true,
        "{\"name\":\"John\",\"age\":30}",
        "JSON body with Content-Length should parse correctly"
    ));
    
    // ========================================================================
    // 2. CHUNKED ENCODING BODY TESTS
    // ========================================================================
    
    test_cases.push_back(BodyTestCase(
        "chunked_simple",
        "POST /upload HTTP/1.1\r\nHost: example.com\r\nTransfer-Encoding: chunked\r\n\r\n",
        "7\r\nMozilla\r\n9\r\nDeveloper\r\n0\r\n\r\n",
        true,
        "MozillaDeveloper",
        "Simple chunked body should parse and concatenate correctly"
    ));
    
    test_cases.push_back(BodyTestCase(
        "chunked_empty_only",
        "POST /upload HTTP/1.1\r\nHost: example.com\r\nTransfer-Encoding: chunked\r\n\r\n",
        "0\r\n\r\n",
        true,
        "",
        "Chunked body with only final chunk should result in empty body"
    ));
    
    test_cases.push_back(BodyTestCase(
        "chunked_single_chunk",
        "POST /upload HTTP/1.1\r\nHost: example.com\r\nTransfer-Encoding: chunked\r\n\r\n",
        "5\r\nHello\r\n0\r\n\r\n",
        true,
        "Hello",
        "Single chunk should parse correctly"
    ));
    
    test_cases.push_back(BodyTestCase(
        "chunked_with_trailer",
        "POST /upload HTTP/1.1\r\nHost: example.com\r\nTransfer-Encoding: chunked\r\n\r\n",
        "7\r\nMozilla\r\n0\r\nExpires: Wed, 21 Oct 2015 07:28:00 GMT\r\n\r\n",
        true,
        "Mozilla",
        "Chunked body with trailer headers should parse correctly"
    ));
    
    test_cases.push_back(BodyTestCase(
        "chunked_hex_uppercase",
        "POST /upload HTTP/1.1\r\nHost: example.com\r\nTransfer-Encoding: chunked\r\n\r\n",
        "A\r\nHelloWorld\r\n0\r\n\r\n",
        true,
        "HelloWorld",
        "Uppercase hex chunk size should parse correctly"
    ));
    
    test_cases.push_back(BodyTestCase(
        "chunked_large_chunk",
        "POST /upload HTTP/1.1\r\nHost: example.com\r\nTransfer-Encoding: chunked\r\n\r\n",
        "FF\r\n" + std::string(255, 'A') + "\r\n0\r\n\r\n",
        true,
        std::string(255, 'A'),
        "Large chunk (255 bytes) should parse correctly"
    ));
    
    // ========================================================================
    // 3. GET/DELETE BODY TESTS (Should reject bodies)
    // ========================================================================
    
    test_cases.push_back(BodyTestCase(
        "get_with_content_length",
        "GET /index.html HTTP/1.1\r\nHost: example.com\r\nContent-Length: 5\r\n\r\n",
        "Hello",
        false,
        "",
        "GET with Content-Length should be invalid"
    ));
    
    test_cases.push_back(BodyTestCase(
        "delete_with_chunked",
        "DELETE /resource HTTP/1.1\r\nHost: example.com\r\nTransfer-Encoding: chunked\r\n\r\n",
        "5\r\nHello\r\n0\r\n\r\n",
        false,
        "",
        "DELETE with chunked encoding should be invalid"
    ));
    
    test_cases.push_back(BodyTestCase(
        "get_no_body",
        "GET /index.html HTTP/1.1\r\nHost: example.com\r\n\r\n",
        "",
        true,
        "",
        "GET without body should parse correctly"
    ));
    
    // ========================================================================
    // 4. ERROR CASES
    // ========================================================================
    
    test_cases.push_back(BodyTestCase(
        "conflicting_headers",
        "POST /upload HTTP/1.1\r\nHost: example.com\r\nTransfer-Encoding: chunked\r\nContent-Length: 10\r\n\r\n",
        "5\r\nHello\r\n0\r\n\r\n",
        false,
        "",
        "Conflicting Transfer-Encoding and Content-Length should fail"
    ));
    
    test_cases.push_back(BodyTestCase(
        "post_no_body_headers",
        "POST /submit HTTP/1.1\r\nHost: example.com\r\n\r\n",
        "Some body data",
        false,
        "",
        "POST without Content-Length or Transfer-Encoding should fail"
    ));
    
    test_cases.push_back(BodyTestCase(
        "chunked_invalid_hex",
        "POST /upload HTTP/1.1\r\nHost: example.com\r\nTransfer-Encoding: chunked\r\n\r\n",
        "XYZ\r\nHello\r\n0\r\n\r\n",
        false,
        "",
        "Invalid hex chunk size should fail"
    ));
    
    test_cases.push_back(BodyTestCase(
        "chunked_missing_final_crlf",
        "POST /upload HTTP/1.1\r\nHost: example.com\r\nTransfer-Encoding: chunked\r\n\r\n",
        "5\r\nHello\r\n0\r\n",  // Missing final \r\n
        false,
        "",
        "Chunked body without proper termination should fail"
    ));
    
    test_cases.push_back(BodyTestCase(
        "chunked_size_data_mismatch",
        "POST /upload HTTP/1.1\r\nHost: example.com\r\nTransfer-Encoding: chunked\r\n\r\n",
        "5\r\nHelloWorld\r\n0\r\n\r\n",  // Says 5 bytes but has 10
        false,
        "",
        "Chunk size not matching data length should fail"
    ));
    
    // ========================================================================
    // 5. EDGE CASES
    // ========================================================================
    
    test_cases.push_back(BodyTestCase(
        "content_length_exact_max",
        "POST /upload HTTP/1.1\r\nHost: example.com\r\nContent-Length: 100\r\n\r\n",
        std::string(100, 'X'),
        true,
        std::string(100, 'X'),
        "Content-Length at reasonable size should work"
    ));
    
    test_cases.push_back(BodyTestCase(
        "chunked_multiple_small",
        "POST /upload HTTP/1.1\r\nHost: example.com\r\nTransfer-Encoding: chunked\r\n\r\n",
        "1\r\nH\r\n1\r\ne\r\n1\r\nl\r\n1\r\nl\r\n1\r\no\r\n0\r\n\r\n",
        true,
        "Hello",
        "Multiple small chunks should concatenate correctly"
    ));
    
    return test_cases;
}

// ============================================================================
// TEST EXECUTION FUNCTIONS
// ============================================================================

void testParseBody() {
    std::cout << "\n" << std::string(50, '-') << std::endl;
    std::cout << "TESTING BODY PARSING" << std::endl;
    std::cout << std::string(50, '-') << std::endl;
    
    std::vector<BodyTestCase> test_cases = createBodyTestCases();
    int passed = 0;
    int total = test_cases.size();
    
    for (size_t i = 0; i < test_cases.size(); i++) {
        const BodyTestCase& test = test_cases[i];
        std::cout << "DEBUG: Starting test " << (i + 1) << ": " << test.name << std::endl;
        std::cout << "DEBUG: " << test.description << std::endl;
        
        try {
            HttpRequest request;
            
            // Setup: Parse request components in proper order
            size_t header_end = test.request_data.find("\r\n\r\n");
            if (header_end != std::string::npos) {
                // Parse request line
                size_t first_crlf = test.request_data.find("\r\n");
                std::string request_line = test.request_data.substr(0, first_crlf);
                request.parseRequestLine(request_line);
                
                // Parse headers (this now sets chunked_encoding_ and content_length_)
                std::string header_section = test.request_data.substr(first_crlf + 2, header_end - first_crlf - 2);
                request.parseHeaders(header_section);
            }
            
            // Test: Parse the body
            bool result = request.parseBody(test.body_data);
            
            // Check result
            if (result == test.expected_result) {
                // If parsing succeeded, also check body content
                if (result && request.getBody() == test.expected_body) {
                    std::cout << "✅ PASS: " << test.name << std::endl;
                    passed++;
                } else if (!result) {
                    std::cout << "✅ PASS: " << test.name << " (correctly failed)" << std::endl;
                    passed++;
                } else {
                    std::cout << "❌ FAIL: " << test.name << " (wrong body content)" << std::endl;
                    std::cout << "   Expected body: [" << test.expected_body << "]" << std::endl;
                    std::cout << "   Got body: [" << request.getBody() << "]" << std::endl;
                }
            } else {
                std::cout << "❌ FAIL: " << test.name << std::endl;
                std::cout << "   Expected: " << boolToString(test.expected_result) << std::endl;
                std::cout << "   Got: " << boolToString(result) << std::endl;
                std::cout << "   Body data: [" << test.body_data << "]" << std::endl;
            }
        } catch (const std::exception& e) {
            std::cout << "❌ EXCEPTION in test " << test.name << ": " << e.what() << std::endl;
            std::cout << "   Body data was: [" << test.body_data << "]" << std::endl;
        }
        
        std::cout << std::endl;
    }
    
    std::cout << "\nBody Parsing Results: " << passed << "/" << total << " passed" << std::endl;
}

// ============================================================================
// MAIN FUNCTION
// ============================================================================

int main() {
    std::cout << std::string(60, '=') << std::endl;
    std::cout << "BODY PARSING TEST SUITE" << std::endl;
    std::cout << std::string(60, '=') << std::endl;

    testParseBody();

    return 0;
}