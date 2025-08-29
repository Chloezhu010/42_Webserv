#include <string>
#include <vector>
#include <iostream>
#include <cassert>
#include "../http/http_request.hpp"

// ============================================================================
// TEST DATA STRUCTURE
// ============================================================================

struct HeaderTestCase {
    std::string name;
    std::string data;
    bool should_pass;
    std::string description;
    
    HeaderTestCase(const std::string& n, const std::string& d, bool pass, const std::string& desc)
        : name(n), data(d), should_pass(pass), description(desc) {}
};

// ============================================================================
// TEST CASE DEFINITIONS
// ============================================================================

std::vector<HeaderTestCase> createHeaderTestCases() {
    std::vector<HeaderTestCase> test_cases;
    
    // 1. BASIC FORMAT ISSUES
    test_cases.push_back(HeaderTestCase(
        "missing_colon",
        "Host example.com\r\n"
        "Content-Type: text/html\r\n",
        false,
        "Header line without colon should fail"
    ));
    
    test_cases.push_back(HeaderTestCase(
        "multiple_colons",
        "Host: example.com\r\n"
        "Time: 12:34:56\r\n"
        "URL: http://example.com:8080\r\n",
        true,
        "Multiple colons in header values should pass"
    ));
    
    test_cases.push_back(HeaderTestCase(
        "empty_header_name",
        "Host: example.com\r\n"
        ": some-value\r\n",
        false,
        "Empty header name should fail"
    ));
    
    test_cases.push_back(HeaderTestCase(
        "empty_header_value",
        "Host: example.com\r\n"
        "Content-Length:\r\n"
        "Accept:\r\n",
        true,
        "Empty header values should pass parsing"
    ));
    
    // 2. WHITESPACE EDGE CASES
    test_cases.push_back(HeaderTestCase(
        "spaces_in_values",
        "Host:   example.com   \r\n"
        "User-Agent:    Mozilla/5.0    \r\n"
        "Content-Type:text/html\r\n",
        true,
        "Leading/trailing spaces in values should be trimmed"
    ));
    
    test_cases.push_back(HeaderTestCase(
        "spaces_in_names",
        "Host Name: example.com\r\n"
        "Content Type: text/html\r\n",
        false,
        "Spaces in header names should fail"
    ));
    
    test_cases.push_back(HeaderTestCase(
        "tab_in_name",
        "Host: example.com\r\n"
        "Content\tType: text/html\r\n",
        false,
        "Tab characters in header names should fail"
    ));
    
    // 3. CASE SENSITIVITY
    test_cases.push_back(HeaderTestCase(
        "case_variations",
        "HOST: example.com\r\n"
        "Content-TYPE: text/html\r\n",
        true,
        "Different case variations should be normalized"
    ));
    
    // 4. SIZE LIMITS
    test_cases.push_back(HeaderTestCase(
        "long_header_line",
        "Host: example.com\r\n"
        "Very-Long-Header: " + std::string(9000, 'a') + "\r\n",
        false,
        "Headers exceeding MAX_HEADER_SIZE should fail"
    ));
    
    // 5. SPECIAL CHARACTERS
    test_cases.push_back(HeaderTestCase(
        "valid_special_chars",
        "Host: example.com\r\n"
        "X-Custom-Header: value\r\n"
        "Content-MD5: abc123\r\n",
        true,
        "Valid special characters in header names should pass"
    ));
    
    test_cases.push_back(HeaderTestCase(
        "invalid_special_chars",
        "Host: example.com\r\n"
        "Bad(Header): value\r\n",
        false,
        "Invalid special characters in header names should fail"
    ));
    
    // 6. BOUNDARY CASES
    test_cases.push_back(HeaderTestCase(
        "just_colon",
        ":\r\n",
        false,
        "Header with only colon should fail"
    ));
    
    test_cases.push_back(HeaderTestCase(
        "empty_headers",
        "",
        true,
        "Empty header section should pass"
    ));
    
    test_cases.push_back(HeaderTestCase(
        "valid_headers",
        "Host: example.com\r\n"
        "User-Agent: Mozilla/5.0\r\n"
        "Accept: text/html\r\n",
        true,
        "Normal well-formed headers should pass"
    ));
    
    test_cases.push_back(HeaderTestCase(
        "minimal_http11",
        "Host: example.com\r\n",
        true,
        "Minimal HTTP/1.1 headers should pass"
    ));
    
    // 7. LINE ENDINGS
    test_cases.push_back(HeaderTestCase(
        "mixed_line_endings",
        "Host: example.com\r\n"    // CRLF
        "Content-Type: text/html\n"  // LF only
        "Accept: text/html\r\n",     // CRLF
        true,
        "Mixed line endings should be handled gracefully"
    ));
    
    return test_cases;
}

// ============================================================================
// TEST UTILITIES
// ============================================================================

void printTestResult(const std::string& test_name, bool passed, const std::string& description) {
    if (passed) {
        std::cout << "✅ PASS: " << test_name << " - " << description << std::endl;
    } else {
        std::cout << "❌ FAIL: " << test_name << " - " << description << std::endl;
    }
}

void printTestSummary(int passed, int total) {
    std::cout << "\n" << std::string(60, '=') << std::endl;
    std::cout << "TEST SUMMARY" << std::endl;
    std::cout << std::string(60, '=') << std::endl;
    std::cout << "Passed: " << passed << "/" << total << std::endl;
    std::cout << "Failed: " << (total - passed) << "/" << total << std::endl;
    std::cout << "Success Rate: " << (total > 0 ? (100.0 * passed / total) : 0) << "%" << std::endl;
    
    if (passed == total) {
        std::cout << "🎉 ALL TESTS PASSED!" << std::endl;
    } else {
        std::cout << "⚠️  Some tests failed. Review implementation." << std::endl;
    }
}


// static std::vector<std::string> splitIntoLines(const std::string& str)
// {
//     std::vector<std::string> results;
//     // if the string is empty, return empty vector
//     if (str.empty())
//         return results;
//     // split by \r\n, handle cases with only \n or only \r
//     size_t start = 0;
//     size_t end = 0;
//     while (end < str.length())
//     {
//         // standard line ending \r\n
//         if (str[end] == '\r' && end + 1 < str.length() && str[end + 1] == '\n')
//         {
//             results.push_back(str.substr(start, end - start));
//             end += 2; // skip \r\n
//             start = end;
//         }
//         // handle lone \n
//         else if (str[end] == '\n') 
//         {
//             results.push_back(str.substr(start, end - start));
//             end += 1; // skip \n
//             start = end;
//         }
//         // handle lone \r
//         else if (str[end] == '\r')
//         {
//             results.push_back(str.substr(start, end - start));
//             end += 1; // skip \r
//             start = end;
//         }
//         else
//         {
//             end++;
//         }
//     }
//     // add the last line if any
//     if (start < str.length())
//         results.push_back(str.substr(start));
    
//     return results;
// }

// /* test splitIntoLines */
// int main()
// {
//     // Test 1: Standard CRLF
//     std::string test1 = "Host: example.com\r\nContent-Type: text/html\r\n";
//     std::vector<std::string> result1 = splitIntoLines(test1);
//     assert(result1.size() == 2);
//     assert(result1[0] == "Host: example.com");
//     assert(result1[1] == "Content-Type: text/html");
    
//     // Test 2: Mixed line endings
//     std::string test2 = "Host: example.com\r\nContent-Type: text/html\nUser-Agent: Mozilla\r";
//     std::vector<std::string> result2 = splitIntoLines(test2);
//     assert(result2.size() == 3);
//     assert(result2[0] == "Host: example.com");
//     assert(result2[1] == "Content-Type: text/html");
//     assert(result2[2] == "User-Agent: Mozilla");
    
//     // Test 3: No final line ending
//     std::string test3 = "Host: example.com\r\nContent-Type: text/html";
//     std::vector<std::string> result3 = splitIntoLines(test3);
//     assert(result3.size() == 2);
//     assert(result3[1] == "Content-Type: text/html");
    
//     // Test 4: Empty lines
//     std::string test4 = "Host: example.com\r\n\r\nContent-Type: text/html\r\n";
//     std::vector<std::string> result4 = splitIntoLines(test4);
//     assert(result4.size() == 3);
//     assert(result4[0] == "Host: example.com");
//     assert(result4[1] == ""); // Empty line
//     assert(result4[2] == "Content-Type: text/html");
    
//     // Test 5: Only CRLF
//     std::string test5 = "\r\n";
//     std::vector<std::string> result5 = splitIntoLines(test5);
//     assert(result5.size() == 1);
//     assert(result5[0] == "");
    
//     // Test 6: Empty string
//     std::string test6 = "";
//     std::vector<std::string> result6 = splitIntoLines(test6);
//     assert(result6.size() == 0);
    
//     std::cout << "All tests passed!" << std::endl;
// }


void testHeaderParsing() {
    std::cout << "\n" << std::string(40, '-') << std::endl;
    std::cout << "Testing parseHeaders function" << std::endl;
    std::cout << std::string(40, '-') << std::endl;
    
    std::vector<HeaderTestCase> test_cases = createHeaderTestCases();
    HttpRequest req;
    
    int passed = 0;
    int total = test_cases.size();
    
    for (size_t i = 0; i < test_cases.size(); ++i) {
        const HeaderTestCase& test = test_cases[i];
        std::cout << "DEBUG: Starting test " << i << ": " << test.name << std::endl;
        std::cout << "DEBUG: Test data: [" << test.data << "]" << std::endl;
        
        try {
            // Clear any previous state
            HttpRequest fresh_req;
            
            bool result = fresh_req.parseHeaders(test.data);
            bool test_passed = (result == test.should_pass);
            
            if (test_passed) {
                passed++;
            }
            
            printTestResult(test.name, test_passed, test.description);
            
        } catch (const std::exception& e) {
            std::cout << "❌ EXCEPTION in test " << test.name << ": " << e.what() << std::endl;
            std::cout << "    Test data was: [" << test.data << "]" << std::endl;
            return; // Stop testing on exception
        }
        // // Clear any previous state
        // HttpRequest fresh_req;
        
        // bool result = fresh_req.parseHeaders(test.data);
        // bool test_passed = (result == test.should_pass);
        
        // if (test_passed) {
        //     passed++;
        // }
        
        // printTestResult(test.name, test_passed, test.description);
        
        // // Additional debug info for failures
        // if (!test_passed) {
        //     std::cout << "    Expected: " << (test.should_pass ? "PASS" : "FAIL") << std::endl;
        //     std::cout << "    Got:      " << (result ? "PASS" : "FAIL") << std::endl;
        //     std::cout << "    Data:     \"" << test.data.substr(0, 50) << "...\"" << std::endl;
        // }
    }
    
    std::cout << "\nparseHeaders Results: " << passed << "/" << total << " passed" << std::endl;
    return;
}

int main()
{
    std::cout << std::string(60, '=') << std::endl;
    std::cout << "HTTP HEADER PARSING TEST SUITE" << std::endl;
    std::cout << std::string(60, '=') << std::endl;

    testHeaderParsing();


    return 0;
}