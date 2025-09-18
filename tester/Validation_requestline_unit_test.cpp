#include <string>
#include <vector>
#include <iostream>
#include <cassert>
#include "../http/http_request.hpp"

// ============================================================================
// TEST DATA STRUCTURE
// ============================================================================

struct ValidationTestCase {
    std::string name;
    std::string complete_request;
    ValidationResult expected_result;
    std::string description;
    
    ValidationTestCase(const std::string& n, const std::string& req, ValidationResult result, const std::string& desc = "")
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
        else result += str[i];
    }
    return result;
}

// ============================================================================
// TEST FUNCTIONS
// ============================================================================

bool runValidationTest(const ValidationTestCase& test) {
    HttpRequest request;
    
    // Parse the request first (required for validation)
    bool parse_result = request.parseRequest(test.complete_request);
    // std::cout << "parse result: " << parse_result << std::endl;
    // std::cout << "is_complete: " << request.getIsComplete() << std::endl;

    if (!parse_result) {
        std::cout << "PARSE FAILED for test: " << test.name << std::endl;
        return false;
    }
    
    // Run validation
    ValidationResult actual_result = request.validateRequest();
    
    bool success = (actual_result == test.expected_result);
    
    if (!success) {
        std::cout << "VALIDATION TEST FAILED: " << test.name << std::endl;
        std::cout << "  Description: " << test.description << std::endl;
        std::cout << "  Request: " << escapeString(test.complete_request) << std::endl;
        std::cout << "  Expected: " << validationResultToString(test.expected_result) << std::endl;
        std::cout << "  Actual: " << validationResultToString(actual_result) << std::endl;
        std::cout << std::endl;
    }
    
    return success;
}

bool runInputValidationTest(const ValidationTestCase& test) {
    HttpRequest request;
    
    // Parse the request first (required for validation)
    bool parse_result = request.parseRequest(test.complete_request);
    // std::cout << "parse result: " << parse_result << std::endl;
    // std::cout << "is_complete: " << request.getIsComplete() << std::endl;

    if (!parse_result) {
        std::cout << "PARSE FAILED for input validation test: " << test.name << std::endl;
        return false;
    }
    
    // Run input validation specifically
    ValidationResult actual_result = request.inputValidation();
    
    bool success = (actual_result == test.expected_result);
    
    if (!success) {
        std::cout << "INPUT VALIDATION TEST FAILED: " << test.name << std::endl;
        std::cout << "  Description: " << test.description << std::endl;
        std::cout << "  Expected: " << validationResultToString(test.expected_result) << std::endl;
        std::cout << "  Actual: " << validationResultToString(actual_result) << std::endl;
        std::cout << std::endl;
    }
    
    return success;
}

bool runValidateRequestLineTest(const ValidationTestCase& test) {
    HttpRequest request;
    
    // Parse the request first (required for validation)
    bool parse_result = request.parseRequest(test.complete_request);
    // std::cout << "parse result: " << parse_result << std::endl;
    // std::cout << "is_complete: " << request.getIsComplete() << std::endl;

    if (!parse_result) {
        std::cout << "PARSE FAILED for request line validation test: " << test.name << std::endl;
        return false;
    }
    
    // Run request line validation specifically
    ValidationResult actual_result = request.validateRequestLine();
    
    bool success = (actual_result == test.expected_result);
    
    if (!success) {
        std::cout << "REQUEST LINE VALIDATION TEST FAILED: " << test.name << std::endl;
        std::cout << "  Description: " << test.description << std::endl;
        std::cout << "  Expected: " << validationResultToString(test.expected_result) << std::endl;
        std::cout << "  Actual: " << validationResultToString(actual_result) << std::endl;
        std::cout << std::endl;
    }
    
    return success;
}

bool runValidateURITest(const ValidationTestCase& test) {
    HttpRequest request;
    
    // Parse the request first (required for validation)
    bool parse_result = request.parseRequest(test.complete_request);
    // std::cout << "parse result: " << parse_result << std::endl;
    // std::cout << "is_complete: " << request.getIsComplete() << std::endl;

    if (!parse_result) {
        std::cout << "PARSE FAILED for URI validation test: " << test.name << std::endl;
        return false;
    }
    
    // Run URI validation specifically
    ValidationResult actual_result = request.validateURI();
    
    bool success = (actual_result == test.expected_result);
    
    if (!success) {
        std::cout << "URI VALIDATION TEST FAILED: " << test.name << std::endl;
        std::cout << "  Description: " << test.description << std::endl;
        std::cout << "  Expected: " << validationResultToString(test.expected_result) << std::endl;
        std::cout << "  Actual: " << validationResultToString(actual_result) << std::endl;
        std::cout << std::endl;
    }
    
    return success;
}

// ============================================================================
// TEST DATA
// ============================================================================

std::vector<ValidationTestCase> getInputValidationTestCases() {
    std::vector<ValidationTestCase> test_cases;
    
    // Valid cases
    test_cases.push_back(ValidationTestCase(
        "valid_get", 
        "GET /index.html HTTP/1.1\r\nHost: example.com\r\n\r\n",
        VALID_REQUEST,
        "Basic valid GET request"
    ));
    
    test_cases.push_back(ValidationTestCase(
        "valid_post", 
        "POST /api/data HTTP/1.1\r\nHost: example.com\r\nContent-Length: 5\r\n\r\nhello",
        VALID_REQUEST,
        "Basic valid POST request"
    ));
    
    return test_cases;
}

std::vector<ValidationTestCase> getRequestLineValidationTestCases() {
    std::vector<ValidationTestCase> test_cases;
    
    // Valid method tests
    test_cases.push_back(ValidationTestCase(
        "valid_get_method", 
        "GET /index.html HTTP/1.1\r\nHost: example.com\r\n\r\n",
        VALID_REQUEST,
        "Valid GET method"
    ));
    
    test_cases.push_back(ValidationTestCase(
        "valid_post_method", 
        "POST /api/data HTTP/1.1\r\nHost: example.com\r\nContent-Length: 0\r\n\r\n",
        VALID_REQUEST,
        "Valid POST method"
    ));
    
    test_cases.push_back(ValidationTestCase(
        "valid_delete_method", 
        "DELETE /api/data HTTP/1.1\r\nHost: example.com\r\n\r\n",
        VALID_REQUEST,
        "Valid DELETE method"
    ));
    
    // Invalid method tests
    test_cases.push_back(ValidationTestCase(
        "invalid_method_put", 
        "PUT /api/data HTTP/1.1\r\nHost: example.com\r\n\r\n",
        INVALID_METHOD,
        "PUT method not supported"
    ));
    
    test_cases.push_back(ValidationTestCase(
        "invalid_method_head", 
        "HEAD /index.html HTTP/1.1\r\nHost: example.com\r\n\r\n",
        INVALID_METHOD,
        "HEAD method not supported"
    ));
    
    // HTTP version tests
    test_cases.push_back(ValidationTestCase(
        "invalid_http_version_1_0", 
        "GET /index.html HTTP/1.0\r\nHost: example.com\r\n\r\n",
        INVALID_HTTP_VERSION,
        "HTTP/1.0 not supported"
    ));
    
    test_cases.push_back(ValidationTestCase(
        "invalid_http_version_2_0", 
        "GET /index.html HTTP/2.0\r\nHost: example.com\r\n\r\n",
        INVALID_HTTP_VERSION,
        "HTTP/2.0 not supported"
    ));
    
    test_cases.push_back(ValidationTestCase(
        "invalid_http_version_lowercase", 
        "GET /index.html http/1.1\r\nHost: example.com\r\n\r\n",
        INVALID_HTTP_VERSION,
        "Lowercase HTTP version"
    ));
    
    return test_cases;
}

std::vector<ValidationTestCase> getURIValidationTestCases() {
    std::vector<ValidationTestCase> test_cases;
    
    // Valid URI tests
    test_cases.push_back(ValidationTestCase(
        "valid_uri_root", 
        "GET / HTTP/1.1\r\nHost: example.com\r\n\r\n",
        VALID_REQUEST,
        "Root path URI"
    ));
    
    test_cases.push_back(ValidationTestCase(
        "valid_uri_file", 
        "GET /index.html HTTP/1.1\r\nHost: example.com\r\n\r\n",
        VALID_REQUEST,
        "File path URI"
    ));
    
    test_cases.push_back(ValidationTestCase(
        "valid_uri_path", 
        "GET /api/users/123 HTTP/1.1\r\nHost: example.com\r\n\r\n",
        VALID_REQUEST,
        "Multi-level path URI"
    ));
    
    test_cases.push_back(ValidationTestCase(
        "valid_uri_with_query", 
        "GET /search?q=test&limit=10 HTTP/1.1\r\nHost: example.com\r\n\r\n",
        VALID_REQUEST,
        "URI with query parameters"
    ));
    
    test_cases.push_back(ValidationTestCase(
        "valid_uri_percent_encoded", 
        "GET /files/my%20file.txt HTTP/1.1\r\nHost: example.com\r\n\r\n",
        VALID_REQUEST,
        "URI with percent encoding"
    ));
    
    // Invalid URI tests - Path traversal
    test_cases.push_back(ValidationTestCase(
        "invalid_uri_path_traversal", 
        "GET /../etc/passwd HTTP/1.1\r\nHost: example.com\r\n\r\n",
        INVALID_URI,
        "Path traversal attack"
    ));
    
    test_cases.push_back(ValidationTestCase(
        "invalid_uri_encoded_traversal", 
        "GET /..%2fetc%2fpasswd HTTP/1.1\r\nHost: example.com\r\n\r\n",
        INVALID_URI,
        "Encoded path traversal"
    ));
    
    test_cases.push_back(ValidationTestCase(
        "invalid_uri_double_encoded", 
        "GET /%2e%2e/etc/passwd HTTP/1.1\r\nHost: example.com\r\n\r\n",
        INVALID_URI,
        "Double encoded path traversal"
    ));
    
    // Invalid percent encoding
    test_cases.push_back(ValidationTestCase(
        "invalid_uri_bad_percent", 
        "GET /file%GG HTTP/1.1\r\nHost: example.com\r\n\r\n",
        INVALID_URI,
        "Invalid hex in percent encoding"
    ));
    
    test_cases.push_back(ValidationTestCase(
        "invalid_uri_incomplete_percent", 
        "GET /file% HTTP/1.1\r\nHost: example.com\r\n\r\n",
        INVALID_URI,
        "Incomplete percent encoding"
    ));
    
    // Control characters
    test_cases.push_back(ValidationTestCase(
        "invalid_uri_control_char", 
        "GET /file\x01test HTTP/1.1\r\nHost: example.com\r\n\r\n",
        INVALID_URI,
        "Control character in URI"
    ));
    
    return test_cases;
}

// ============================================================================
// MAIN TEST RUNNER
// ============================================================================

int main() {
    std::cout << "=== HTTP REQUEST VALIDATION UNIT TESTS ===" << std::endl;
    
    int total_tests = 0;
    int passed_tests = 0;
    
    // Input Validation Tests
    std::cout << "\n--- Input Validation Tests ---" << std::endl;
    std::vector<ValidationTestCase> input_tests = getInputValidationTestCases();
    for (size_t i = 0; i < input_tests.size(); i++) {
        total_tests++;
        if (runInputValidationTest(input_tests[i])) {
            passed_tests++;
            std::cout << "PASS: " << input_tests[i].name << std::endl;
        }
    }
    
    // Request Line Validation Tests  
    std::cout << "\n--- Request Line Validation Tests ---" << std::endl;
    std::vector<ValidationTestCase> request_line_tests = getRequestLineValidationTestCases();
    for (size_t i = 0; i < request_line_tests.size(); i++) {
        total_tests++;
        if (runValidateRequestLineTest(request_line_tests[i])) {
            passed_tests++;
            std::cout << "PASS: " << request_line_tests[i].name << std::endl;
        }
    }
    
    // URI Validation Tests
    std::cout << "\n--- URI Validation Tests ---" << std::endl;
    std::vector<ValidationTestCase> uri_tests = getURIValidationTestCases();
    for (size_t i = 0; i < uri_tests.size(); i++) {
        total_tests++;
        if (runValidateURITest(uri_tests[i])) {
            passed_tests++;
            std::cout << "PASS: " << uri_tests[i].name << std::endl;
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