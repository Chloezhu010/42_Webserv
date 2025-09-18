#include <string>
#include <vector>
#include <iostream>
#include <cassert>
#include "../http/http_request.hpp"

// ============================================================================
// TEST DATA STRUCTURE
// ============================================================================

struct HeaderValidationTestCase {
    std::string name;
    std::string complete_request;
    ValidationResult expected_result;
    std::string description;
    
    HeaderValidationTestCase(const std::string& n, const std::string& req, ValidationResult result, const std::string& desc = "")
        : name(n), complete_request(req), expected_result(result), description(desc) {}
};

// ============================================================================
// HELPER FUNCTIONS
// ============================================================================

std::string validationResultToString(ValidationResult result) {
    switch (result) {
        case NOT_VALIDATED: return "NOT_VALIDATED";
        case VALID_REQUEST: return "VALID_REQUEST";
        case BAD_REQUEST: return "BAD_REQUEST";
        case INVALID_REQUEST_LINE: return "INVALID_REQUEST_LINE";
        case INVALID_HTTP_VERSION: return "INVALID_HTTP_VERSION";
        case INVALID_URI: return "INVALID_URI";
        case INVALID_METHOD: return "INVALID_METHOD";
        case URI_TOO_LONG: return "URI_TOO_LONG";
        case INVALID_HEADER: return "INVALID_HEADER";
        case METHOD_BODY_MISMATCH: return "METHOD_BODY_MISMATCH";
        case LENGTH_REQUIRED: return "LENGTH_REQUIRED";
        case MISSING_HOST_HEADER: return "MISSING_HOST_HEADER";
        default: return "UNKNOWN";
    }
}

std::string escapeString(const std::string& str) {
    std::string result;
    for (size_t i = 0; i < str.length(); i++) {
        if (str[i] == '\r') result += "\\r";
        else if (str[i] == '\n') result += "\\n";
        else if (str[i] == '\t') result += "\\t";
        else if (str[i] >= 0 && str[i] <= 31) {
            result += "\\x";
            result += (str[i] / 16 < 10) ? ('0' + str[i] / 16) : ('A' + str[i] / 16 - 10);
            result += (str[i] % 16 < 10) ? ('0' + str[i] % 16) : ('A' + str[i] % 16 - 10);
        }
        else result += str[i];
    }
    return result;
}

// ============================================================================
// TEST FUNCTIONS
// ============================================================================

bool runHeaderValidationTest(const HeaderValidationTestCase& test) {
    HttpRequest request;
    
    // Parse the request first (required for validation)
    bool parse_result = request.parseRequest(test.complete_request);
    if (!parse_result) {
        std::cout << "❌ PARSE FAILED for header validation test: " << test.name << std::endl;
        return false;
    }
    
    // Run header validation specifically
    ValidationResult actual_result = request.validateHeader();
    
    bool success = (actual_result == test.expected_result);
    
    if (!success) {
        std::cout << "❌ HEADER VALIDATION TEST FAILED: " << test.name << std::endl;
        std::cout << "  Description: " << test.description << std::endl;
        std::cout << "  Request: " << escapeString(test.complete_request) << std::endl;
        std::cout << "  Expected: " << validationResultToString(test.expected_result) << std::endl;
        std::cout << "  Actual: " << validationResultToString(actual_result) << std::endl;
        std::cout << std::endl;
    }
    
    return success;
}

// ============================================================================
// TEST DATA
// ============================================================================

std::vector<HeaderValidationTestCase> getHeaderValidationTestCases() {
    std::vector<HeaderValidationTestCase> test_cases;
    
    // ===== HOST HEADER VALIDATION =====
    
    // Valid host header tests
    test_cases.push_back(HeaderValidationTestCase(
        "valid_host_simple", 
        "GET /index.html HTTP/1.1\r\nHost: example.com\r\n\r\n",
        VALID_REQUEST,
        "Valid host header with domain"
    ));
    
    test_cases.push_back(HeaderValidationTestCase(
        "valid_host_with_port", 
        "GET /index.html HTTP/1.1\r\nHost: example.com:8080\r\n\r\n",
        VALID_REQUEST,
        "Valid host header with port"
    ));
    
    test_cases.push_back(HeaderValidationTestCase(
        "valid_host_ip", 
        "GET /index.html HTTP/1.1\r\nHost: 192.168.1.1\r\n\r\n",
        VALID_REQUEST,
        "Valid host header with IP address"
    ));
    
    // Invalid host header tests
    test_cases.push_back(HeaderValidationTestCase(
        "invalid_host_empty", 
        "GET /index.html HTTP/1.1\r\nHost: \r\n\r\n",
        INVALID_HEADER,
        "Empty host header value"
    ));
    
    // ===== CONTENT-LENGTH VALIDATION =====
    
    // Valid Content-Length tests
    test_cases.push_back(HeaderValidationTestCase(
        "valid_post_content_length", 
        "POST /api/data HTTP/1.1\r\nHost: example.com\r\nContent-Length: 5\r\n\r\nhello",
        VALID_REQUEST,
        "Valid POST with Content-Length"
    ));
    
    test_cases.push_back(HeaderValidationTestCase(
        "valid_post_content_length_zero", 
        "POST /api/data HTTP/1.1\r\nHost: example.com\r\nContent-Length: 0\r\n\r\n",
        VALID_REQUEST,
        "Valid POST with zero Content-Length"
    ));
    
    test_cases.push_back(HeaderValidationTestCase(
        "valid_get_no_content_length", 
        "GET /index.html HTTP/1.1\r\nHost: example.com\r\n\r\n",
        VALID_REQUEST,
        "Valid GET without Content-Length"
    ));
    
    // Invalid Content-Length tests
    test_cases.push_back(HeaderValidationTestCase(
        "invalid_get_with_content_length", 
        "GET /index.html HTTP/1.1\r\nHost: example.com\r\nContent-Length: 100\r\n\r\n",
        METHOD_BODY_MISMATCH,
        "GET method with Content-Length (should not have body)"
    ));
    
    test_cases.push_back(HeaderValidationTestCase(
        "invalid_delete_with_content_length", 
        "DELETE /api/data HTTP/1.1\r\nHost: example.com\r\nContent-Length: 50\r\n\r\n",
        METHOD_BODY_MISMATCH,
        "DELETE method with Content-Length (should not have body)"
    ));
    
    test_cases.push_back(HeaderValidationTestCase(
        "invalid_post_no_body_info", 
        "POST /api/data HTTP/1.1\r\nHost: example.com\r\n\r\n",
        LENGTH_REQUIRED,
        "POST without Content-Length or Transfer-Encoding"
    ));
    
    // Multiple Content-Length headers test
    test_cases.push_back(HeaderValidationTestCase(
        "invalid_multiple_content_length", 
        "POST /api/data HTTP/1.1\r\nHost: example.com\r\nContent-Length: 5\r\nContent-Length: 10\r\n\r\nhello",
        INVALID_HEADER,
        "Multiple Content-Length headers"
    ));
    
    // ===== HEADER FORMAT VALIDATION =====
    
    // Valid header name/value tests
    test_cases.push_back(HeaderValidationTestCase(
        "valid_custom_header", 
        "GET /index.html HTTP/1.1\r\nHost: example.com\r\nX-Custom-Header: custom-value\r\n\r\n",
        VALID_REQUEST,
        "Valid custom header with hyphens"
    ));
    
    test_cases.push_back(HeaderValidationTestCase(
        "valid_header_special_chars", 
        "GET /index.html HTTP/1.1\r\nHost: example.com\r\nX-Header_With.Special+Chars: value\r\n\r\n",
        VALID_REQUEST,
        "Valid header name with RFC allowed special characters"
    ));
    
    test_cases.push_back(HeaderValidationTestCase(
        "valid_header_value_spaces", 
        "GET /index.html HTTP/1.1\r\nHost: example.com\r\nUser-Agent: Mozilla/5.0 (compatible)\r\n\r\n",
        VALID_REQUEST,
        "Valid header value with spaces"
    ));
    
    // Invalid header name tests
    test_cases.push_back(HeaderValidationTestCase(
        "invalid_header_name_space", 
        "GET /index.html HTTP/1.1\r\nHost: example.com\r\nInvalid Header: value\r\n\r\n",
        INVALID_HEADER,
        "Invalid header name with space"
    ));
    
    test_cases.push_back(HeaderValidationTestCase(
        "invalid_header_name_control", 
        "GET /index.html HTTP/1.1\r\nHost: example.com\r\nHeader\x01Name: value\r\n\r\n",
        INVALID_HEADER,
        "Invalid header name with control character"
    ));
    
    
    // Invalid header value tests
    test_cases.push_back(HeaderValidationTestCase(
        "invalid_header_value_control", 
        "GET /index.html HTTP/1.1\r\nHost: example.com\r\nX-Header: value\x01invalid\r\n\r\n",
        INVALID_HEADER,
        "Invalid header value with control character"
    ));
    
    test_cases.push_back(HeaderValidationTestCase(
        "invalid_header_value_newline", 
        "GET /index.html HTTP/1.1\r\nHost: example.com\r\nX-Header: value\nwith\nnewline\r\n\r\n",
        INVALID_HEADER,
        "Invalid header value with newline"
    ));
    
    test_cases.push_back(HeaderValidationTestCase(
        "invalid_header_value_null", 
        "GET /index.html HTTP/1.1\r\nHost: example.com\r\nX-Header: value\x00null\r\n\r\n",
        INVALID_HEADER,
        "Invalid header value with null byte"
    ));
    
    // ===== TRANSFER-ENCODING CHUNKED TESTS =====
    
    // Valid chunked encoding tests
    test_cases.push_back(HeaderValidationTestCase(
        "valid_post_chunked", 
        "POST /api/data HTTP/1.1\r\nHost: example.com\r\nTransfer-Encoding: chunked\r\n\r\n5\r\nhello\r\n0\r\n\r\n",
        VALID_REQUEST,
        "Valid POST with chunked encoding"
    ));
    
    test_cases.push_back(HeaderValidationTestCase(
        "valid_chunked_case_insensitive", 
        "POST /api/data HTTP/1.1\r\nHost: example.com\r\nTransfer-Encoding: CHUNKED\r\n\r\n5\r\nhello\r\n0\r\n\r\n",
        VALID_REQUEST,
        "Valid chunked encoding (case insensitive)"
    ));
    
    test_cases.push_back(HeaderValidationTestCase(
        "valid_chunked_with_spaces", 
        "POST /api/data HTTP/1.1\r\nHost: example.com\r\nTransfer-Encoding:  chunked  \r\n\r\n5\r\nhello\r\n0\r\n\r\n",
        VALID_REQUEST,
        "Valid chunked encoding with spaces"
    ));
    
    // Invalid Transfer-Encoding tests
    test_cases.push_back(HeaderValidationTestCase(
        "invalid_transfer_encoding_gzip", 
        "POST /api/data HTTP/1.1\r\nHost: example.com\r\nTransfer-Encoding: gzip\r\n\r\n",
        INVALID_HEADER,
        "Unsupported transfer encoding (gzip)"
    ));
    
    test_cases.push_back(HeaderValidationTestCase(
        "invalid_transfer_encoding_deflate", 
        "POST /api/data HTTP/1.1\r\nHost: example.com\r\nTransfer-Encoding: deflate\r\n\r\n",
        INVALID_HEADER,
        "Unsupported transfer encoding (deflate)"
    ));
    
    test_cases.push_back(HeaderValidationTestCase(
        "invalid_transfer_encoding_multiple", 
        "POST /api/data HTTP/1.1\r\nHost: example.com\r\nTransfer-Encoding: chunked, gzip\r\n\r\n",
        INVALID_HEADER,
        "Multiple transfer encodings (only chunked supported)"
    ));
    
    test_cases.push_back(HeaderValidationTestCase(
        "invalid_transfer_encoding_unknown", 
        "POST /api/data HTTP/1.1\r\nHost: example.com\r\nTransfer-Encoding: custom-encoding\r\n\r\n",
        INVALID_HEADER,
        "Unknown transfer encoding"
    ));
    
    // Multiple Transfer-Encoding headers
    test_cases.push_back(HeaderValidationTestCase(
        "invalid_multiple_transfer_encoding", 
        "POST /api/data HTTP/1.1\r\nHost: example.com\r\nTransfer-Encoding: chunked\r\nTransfer-Encoding: chunked\r\n\r\n5\r\nhello\r\n0\r\n\r\n",
        INVALID_HEADER,
        "Multiple Transfer-Encoding headers"
    ));
    
    // Conflicting headers (Transfer-Encoding + Content-Length)
    test_cases.push_back(HeaderValidationTestCase(
        "invalid_chunked_with_content_length", 
        "POST /api/data HTTP/1.1\r\nHost: example.com\r\nTransfer-Encoding: chunked\r\nContent-Length: 100\r\n\r\n5\r\nhello\r\n0\r\n\r\n",
        CONFLICTING_HEADER,
        "Transfer-Encoding chunked with Content-Length (conflicting)"
    ));
    
    // GET/DELETE with Transfer-Encoding (should not have body)
    test_cases.push_back(HeaderValidationTestCase(
        "invalid_get_with_chunked", 
        "GET /index.html HTTP/1.1\r\nHost: example.com\r\nTransfer-Encoding: chunked\r\n\r\n",
        METHOD_BODY_MISMATCH,
        "GET method with Transfer-Encoding (should not have body)"
    ));
    
    test_cases.push_back(HeaderValidationTestCase(
        "invalid_delete_with_chunked", 
        "DELETE /api/data HTTP/1.1\r\nHost: example.com\r\nTransfer-Encoding: chunked\r\n\r\n",
        METHOD_BODY_MISMATCH,
        "DELETE method with Transfer-Encoding (should not have body)"
    ));
    
    return test_cases;
}

// ============================================================================
// MAIN TEST RUNNER
// ============================================================================

int main() {
    std::cout << "=== HTTP HEADER VALIDATION UNIT TESTS ===" << std::endl;
    
    int total_tests = 0;
    int passed_tests = 0;
    
    // Header Validation Tests
    std::cout << "\n--- Header Validation Tests ---" << std::endl;
    std::vector<HeaderValidationTestCase> header_tests = getHeaderValidationTestCases();
    for (size_t i = 0; i < header_tests.size(); i++) {
        total_tests++;
        if (runHeaderValidationTest(header_tests[i])) {
            passed_tests++;
            std::cout << "✅ PASS: " << header_tests[i].name << std::endl;
        }
    }
    
    // Summary
    std::cout << "\n=== TEST SUMMARY ===" << std::endl;
    std::cout << "Total tests: " << total_tests << std::endl;
    std::cout << "Passed: " << passed_tests << std::endl;
    std::cout << "Failed: " << (total_tests - passed_tests) << std::endl;
    std::cout << "Success rate: " << (passed_tests * 100.0 / total_tests) << "%" << std::endl;
    
    return (passed_tests == total_tests) ? 0 : 1;
}