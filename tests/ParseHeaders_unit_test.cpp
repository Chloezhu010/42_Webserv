#include <string>
#include <vector>
#include <iostream>
#include <cassert>

// Test strings for HTTP header parsing edge cases
void generateHeaderTestCases() {
    
    // ============================================================================
    // 1. BASIC FORMAT ISSUES
    // ============================================================================
    
    // Missing colon - should FAIL parsing
    std::string test_missing_colon = 
        "Host example.com\r\n"
        "Content-Type: text/html\r\n";
    
    // Multiple colons in value - should PASS parsing
    std::string test_multiple_colons = 
        "Host: example.com\r\n"
        "Time: 12:34:56\r\n"
        "URL: http://example.com:8080\r\n"
        "Proxy-Authorization: Basic dXNlcjpwYXNzOndvcmQ=\r\n";
    
    // Empty header name - should FAIL parsing
    std::string test_empty_header_name = 
        "Host: example.com\r\n"
        ": some-value\r\n"
        "Content-Type: text/html\r\n";
    
    // Empty header value - should PASS parsing
    std::string test_empty_header_value = 
        "Host: example.com\r\n"
        "Content-Length:\r\n"
        "Accept:\r\n"
        "User-Agent: \r\n";
    
    // ============================================================================
    // 2. WHITESPACE EDGE CASES
    // ============================================================================
    
    // Leading/trailing spaces in values - should PASS parsing (trim spaces)
    std::string test_spaces_in_values = 
        "Host:   example.com   \r\n"
        "User-Agent:    Mozilla/5.0    \r\n"
        "Content-Type:text/html\r\n"
        "Accept: \ttext/html\t \r\n";
    
    // Spaces in header names - should FAIL parsing
    std::string test_spaces_in_names = 
        "Host Name: example.com\r\n"
        "Content Type: text/html\r\n"
        "User Agent: Mozilla/5.0\r\n";
    
    // Tab characters - mixed scenarios
    std::string test_tab_characters = 
        "Host:\texample.com\r\n"          // Tab in value (should pass)
        "Content\tType: text/html\r\n"    // Tab in name (should fail)
        "Accept: \ttext/html\r\n";        // Tab in value (should pass)
    
    // ============================================================================
    // 3. CASE SENSITIVITY
    // ============================================================================
    
    // Different case variations - should PASS parsing (normalize to lowercase)
    std::string test_case_variations = 
        "HOST: example.com\r\n"
        "host: localhost\r\n"
        "Host: mixed.com\r\n"
        "CONTENT-TYPE: text/html\r\n"
        "content-type: application/json\r\n"
        "Content-Type: text/plain\r\n";
    
    // ============================================================================
    // 4. DUPLICATE HEADERS
    // ============================================================================
    
    // Duplicate Host headers - should PASS parsing but FAIL validation
    std::string test_duplicate_host = 
        "Host: example.com\r\n"
        "Content-Type: text/html\r\n"
        "Host: different.com\r\n"
        "Accept: text/html\r\n";
    
    // Multiple allowed headers - should PASS parsing
    std::string test_multiple_allowed = 
        "Host: example.com\r\n"
        "Accept: text/html\r\n"
        "Accept: application/json\r\n"
        "Accept: */*\r\n"
        "Cookie: session=abc123\r\n"
        "Cookie: user=john\r\n";
    
    // ============================================================================
    // 5. LINE CONTINUATION (Deprecated HTTP/1.1 feature)
    // ============================================================================
    
    // Folded headers - should PASS parsing (unfold) or FAIL (strict mode)
    std::string test_line_folding = 
        "Host: example.com\r\n"
        "User-Agent: Mozilla/5.0\r\n"
        " (compatible; Chrome/91.0)\r\n"
        "\t(Windows NT 10.0)\r\n"
        "Accept: text/html,\r\n"
        " application/json\r\n";
    
    // ============================================================================
    // 6. SIZE LIMITS
    // ============================================================================
    
    // Very long header value - should FAIL parsing if > MAX_HEADER_SIZE
    std::string very_long_value(9000, 'a'); // 9KB of 'a' characters
    std::string test_long_header = 
        "Host: example.com\r\n"
        "Very-Long-Header: " + very_long_value + "\r\n"
        "Content-Type: text/html\r\n";
    
    // Many headers - should FAIL parsing if > MAX_HEADER_COUNT
    std::string test_many_headers = "Host: example.com\r\n";
    for (int i = 0; i < 150; ++i) {
        test_many_headers += "Header" + std::to_string(i) + ": value" + std::to_string(i) + "\r\n";
    }
    
    // ============================================================================
    // 7. SPECIAL CHARACTERS
    // ============================================================================
    
    // Non-ASCII in header names - should FAIL parsing
    std::string test_nonascii_names = 
        "Host: example.com\r\n"
        "Héader: value\r\n"
        "Cañon: test\r\n"
        "Content-Type: text/html\r\n";
    
    // Non-ASCII in header values - implementation dependent
    std::string test_nonascii_values = 
        "Host: example.com\r\n"
        "Comment: Café\r\n"
        "Description: 测试\r\n"
        "Emoji: 🚀\r\n";
    
    // Control characters - should FAIL parsing
    std::string test_control_chars = 
        "Host: example.com\r\n"
        "Bad-Header: value\x00with\x00nulls\r\n"
        "Another: value\x01\x02\x03\r\n"
        "Embedded-CRLF: value\r\ninjection\r\n";
    
    // ============================================================================
    // 8. MALFORMED STRUCTURE
    // ============================================================================
    
    // Empty lines within headers - should FAIL parsing
    std::string test_empty_lines = 
        "Host: example.com\r\n"
        "\r\n"                           // Empty line terminates headers
        "Content-Type: text/html\r\n";   // This should not be parsed as header
    
    // Only CRLF - should PASS parsing (no headers)
    std::string test_only_crlf = "";
    
    // Single header without final CRLF - should PASS parsing
    std::string test_no_final_crlf = 
        "Host: example.com\r\n"
        "Content-Type: text/html";       // No final \r\n
    
    // ============================================================================
    // 9. DIFFERENT LINE ENDINGS
    // ============================================================================
    
    // Mixed line endings - should PASS parsing (handle gracefully)
    std::string test_mixed_endings = 
        "Host: example.com\r\n"          // Standard CRLF
        "Content-Type: text/html\n"      // LF only (Unix)
        "User-Agent: Mozilla/5.0\r"      // CR only (old Mac)
        "Accept: text/html\r\n";         // Standard CRLF
    
    // ============================================================================
    // 10. VALID TYPICAL HEADERS
    // ============================================================================
    
    // Normal well-formed headers - should PASS parsing
    std::string test_valid_headers = 
        "Host: example.com\r\n"
        "User-Agent: Mozilla/5.0 (Windows NT 10.0; Win64; x64) AppleWebKit/537.36\r\n"
        "Accept: text/html,application/xhtml+xml,application/xml;q=0.9,*/*;q=0.8\r\n"
        "Accept-Language: en-US,en;q=0.5\r\n"
        "Accept-Encoding: gzip, deflate\r\n"
        "Connection: keep-alive\r\n"
        "Content-Type: application/x-www-form-urlencoded\r\n"
        "Content-Length: 25\r\n";
    
    // HTTP/1.1 minimum required - should PASS parsing
    std::string test_minimal_http11 = 
        "Host: example.com\r\n";
    
    // ============================================================================
    // 11. BOUNDARY CASES
    // ============================================================================
    
    // Just colon - should FAIL parsing
    std::string test_just_colon = ":\r\n";
    
    // Header name with valid special chars
    std::string test_valid_special_chars = 
        "Host: example.com\r\n"
        "X-Custom-Header: value\r\n"
        "Content-MD5: abc123\r\n"
        "If-None-Match: \"etag-value\"\r\n";
    
    // Header name with invalid special chars - should FAIL parsing
    std::string test_invalid_special_chars = 
        "Host: example.com\r\n"
        "Bad(Header): value\r\n"
        "Another<Header>: value\r\n"
        "Header@Name: value\r\n"
        "Header Name: value\r\n";        // Space in name
    
    // Extremely long header name - should FAIL parsing
    std::string long_name(3000, 'H');
    std::string test_long_header_name = 
        "Host: example.com\r\n" +
        long_name + ": value\r\n";
    
    // ============================================================================
    // 12. SECURITY-RELATED EDGE CASES
    // ============================================================================
    
    // HTTP Request Smuggling attempts - should FAIL parsing
    std::string test_smuggling_attempt = 
        "Host: example.com\r\n"
        "Content-Length: 10\r\n"
        "Content-Length: 20\r\n"         // Duplicate Content-Length
        "Transfer-Encoding: chunked\r\n"; // Conflicting with Content-Length
    
    // Header injection attempts - should FAIL parsing
    std::string test_header_injection = 
        "Host: example.com\r\n"
        "User-Agent: Mozilla\r\nInjected-Header: malicious\r\n"
        "Accept: text/html\r\n";
    
    // ============================================================================
    // TEST RUNNER EXAMPLE
    // ============================================================================
    
    std::cout << "Header parsing test cases generated!" << std::endl;
    std::cout << "Total test cases: 22" << std::endl;
    
    // Example usage:
    // HttpRequest req;
    // assert(req.parseHeaders(test_valid_headers) == true);
    // assert(req.parseHeaders(test_missing_colon) == false);
    // assert(req.parseHeaders(test_spaces_in_names) == false);
    // etc.
}

// // Helper function to run all tests
// void runHeaderParsingTests(HttpRequest& req) {
//     int passed = 0;
//     int total = 0;
    
//     // Tests that should PASS parsing
//     std::vector<std::pair<std::string, std::string>> pass_tests = {
//         {"Valid headers", "Host: example.com\r\nContent-Type: text/html\r\n"},
//         {"Multiple colons", "Host: example.com\r\nTime: 12:34:56\r\n"},
//         {"Empty values", "Host: example.com\r\nAccept:\r\n"},
//         {"Spaces in values", "Host:  example.com  \r\n"},
//         {"Case variations", "HOST: example.com\r\n"},
//         {"No headers", ""},
//     };
    
//     // Tests that should FAIL parsing
//     std::vector<std::pair<std::string, std::string>> fail_tests = {
//         {"Missing colon", "Host example.com\r\n"},
//         {"Empty header name", ": value\r\n"},
//         {"Spaces in header name", "Host Name: example.com\r\n"},
//         {"Control characters", "Host: example.com\x00\r\n"},
//         {"Invalid chars in name", "Bad(Header): value\r\n"},
//         {"Just colon", ":\r\n"},
//     };
    
//     std::cout << "\n=== Running Header Parsing Tests ===\n" << std::endl;
    
//     for (const auto& test : pass_tests) {
//         total++;
//         if (req.parseHeaders(test.second)) {
//             std::cout << "✅ PASS: " << test.first << std::endl;
//             passed++;
//         } else {
//             std::cout << "❌ FAIL: " << test.first << " (expected to pass)" << std::endl;
//         }
//     }
    
//     for (const auto& test : fail_tests) {
//         total++;
//         if (!req.parseHeaders(test.second)) {
//             std::cout << "✅ PASS: " << test.first << " (correctly rejected)" << std::endl;
//             passed++;
//         } else {
//             std::cout << "❌ FAIL: " << test.first << " (should have been rejected)" << std::endl;
//         }
//     }
    
//     std::cout << "\n=== Test Results ===" << std::endl;
//     std::cout << "Passed: " << passed << "/" << total << std::endl;
//     std::cout << "Success rate: " << (100.0 * passed / total) << "%" << std::endl;
// }

static std::vector<std::string> splitIntoLines(const std::string& str)
{
    std::vector<std::string> results;
    // if the string is empty, return empty vector
    if (str.empty())
        return results;
    // split by \r\n, handle cases with only \n or only \r
    size_t start = 0;
    size_t end = 0;
    while (end < str.length())
    {
        // standard line ending \r\n
        if (str[end] == '\r' && end + 1 < str.length() && str[end + 1] == '\n')
        {
            results.push_back(str.substr(start, end - start));
            end += 2; // skip \r\n
            start = end;
        }
        // handle lone \n
        else if (str[end] == '\n') 
        {
            results.push_back(str.substr(start, end - start));
            end += 1; // skip \n
            start = end;
        }
        // handle lone \r
        else if (str[end] == '\r')
        {
            results.push_back(str.substr(start, end - start));
            end += 1; // skip \r
            start = end;
        }
        else
        {
            end++;
        }
    }
    // add the last line if any
    if (start < str.length())
        results.push_back(str.substr(start));
    
    return results;
}


int main()
{
    // Test 1: Standard CRLF
    std::string test1 = "Host: example.com\r\nContent-Type: text/html\r\n";
    std::vector<std::string> result1 = splitIntoLines(test1);
    assert(result1.size() == 2);
    assert(result1[0] == "Host: example.com");
    assert(result1[1] == "Content-Type: text/html");
    
    // Test 2: Mixed line endings
    std::string test2 = "Host: example.com\r\nContent-Type: text/html\nUser-Agent: Mozilla\r";
    std::vector<std::string> result2 = splitIntoLines(test2);
    assert(result2.size() == 3);
    assert(result2[0] == "Host: example.com");
    assert(result2[1] == "Content-Type: text/html");
    assert(result2[2] == "User-Agent: Mozilla");
    
    // Test 3: No final line ending
    std::string test3 = "Host: example.com\r\nContent-Type: text/html";
    std::vector<std::string> result3 = splitIntoLines(test3);
    assert(result3.size() == 2);
    assert(result3[1] == "Content-Type: text/html");
    
    // Test 4: Empty lines
    std::string test4 = "Host: example.com\r\n\r\nContent-Type: text/html\r\n";
    std::vector<std::string> result4 = splitIntoLines(test4);
    assert(result4.size() == 3);
    assert(result4[0] == "Host: example.com");
    assert(result4[1] == ""); // Empty line
    assert(result4[2] == "Content-Type: text/html");
    
    // Test 5: Only CRLF
    std::string test5 = "\r\n";
    std::vector<std::string> result5 = splitIntoLines(test5);
    assert(result5.size() == 1);
    assert(result5[0] == "");
    
    // Test 6: Empty string
    std::string test6 = "";
    std::vector<std::string> result6 = splitIntoLines(test6);
    assert(result6.size() == 0);
    
    std::cout << "All tests passed!" << std::endl;

}