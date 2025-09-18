#include "../http/http_request.hpp"
#include <iostream>
#include <cassert>

void test_large_header_parsing() {
    std::cout << "=== Testing Large Header Parsing ===" << std::endl;

    HttpRequest request;

    // Generate large header value (9MB)
    std::string large_value(9 * 1024 * 1024, 'x');
    std::string large_request =
        "GET / HTTP/1.1\r\n"
        "Host: localhost\r\n"
        "Large-Header: " + large_value + "\r\n"
        "\r\n";

    std::cout << "Request size: " << large_request.length() << " bytes" << std::endl;
    std::cout << "Header value size: " << large_value.length() << " bytes" << std::endl;

    // Test completeness check
    RequestStatus complete_status = request.isRequestComplete(large_request);
    std::cout << "isRequestComplete: " << complete_status << std::endl;

    if (complete_status != REQUEST_COMPLETE) {
        std::cout << "❌ Request not marked as complete" << std::endl;
        return;
    }

    // Test parsing
    std::cout << "Attempting to parse..." << std::endl;
    bool parse_result = request.parseRequest(large_request);
    std::cout << "parseRequest result: " << (parse_result ? "SUCCESS" : "FAILED") << std::endl;
    std::cout << "is_parsed: " << (request.getIsParsed() ? "true" : "false") << std::endl;

    if (!parse_result) {
        std::cout << "❌ Parsing failed before validation" << std::endl;
        return;
    }

    // Test validation
    std::cout << "Attempting to validate..." << std::endl;
    ValidationResult validation_result = request.validateRequest();
    std::cout << "validateRequest result: " << validation_result << std::endl;

    if (validation_result == PAYLOAD_TOO_LARGE) {
        std::cout << "✅ Correctly identified as PAYLOAD_TOO_LARGE (413)" << std::endl;
    } else {
        std::cout << "❌ Wrong validation result: " << validation_result << std::endl;
    }
}

void test_large_uri_parsing() {
    std::cout << "\n=== Testing Large URI Parsing ===" << std::endl;

    HttpRequest request;

    // Generate large URI (3000 chars - should be URI_TOO_LONG, not parsing failure)
    std::string large_uri(3000, 'a');
    std::string uri_request =
        "GET /" + large_uri + " HTTP/1.1\r\n"
        "Host: localhost\r\n"
        "\r\n";

    std::cout << "Request size: " << uri_request.length() << " bytes" << std::endl;
    std::cout << "URI size: " << large_uri.length() << " chars" << std::endl;

    // Test completeness
    RequestStatus complete_status = request.isRequestComplete(uri_request);
    std::cout << "isRequestComplete: " << complete_status << std::endl;

    if (complete_status != REQUEST_COMPLETE) {
        std::cout << "❌ Request not marked as complete" << std::endl;
        return;
    }

    // Test parsing
    std::cout << "Attempting to parse..." << std::endl;
    bool parse_result = request.parseRequest(uri_request);
    std::cout << "parseRequest result: " << (parse_result ? "SUCCESS" : "FAILED") << std::endl;

    if (!parse_result) {
        std::cout << "❌ Parsing failed - should parse and let validation handle" << std::endl;
        return;
    }

    // Test validation
    std::cout << "Attempting to validate..." << std::endl;
    ValidationResult validation_result = request.validateRequest();
    std::cout << "validateRequest result: " << validation_result << std::endl;

    if (validation_result == URI_TOO_LONG) {
        std::cout << "✅ Correctly identified as URI_TOO_LONG (414)" << std::endl;
    } else {
        std::cout << "❌ Wrong validation result: " << validation_result << std::endl;
    }
}

void test_invalid_method_parsing() {
    std::cout << "\n=== Testing Invalid Method Parsing ===" << std::endl;

    HttpRequest request;
    std::string invalid_method_request =
        "PATCH / HTTP/1.1\r\n"
        "Host: localhost\r\n"
        "\r\n";

    std::cout << "Request: " << invalid_method_request << std::endl;

    // Test parsing
    bool parse_result = request.parseRequest(invalid_method_request);
    std::cout << "parseRequest result: " << (parse_result ? "SUCCESS" : "FAILED") << std::endl;

    if (!parse_result) {
        std::cout << "❌ Parsing failed - PATCH should parse successfully" << std::endl;
        return;
    }

    std::cout << "Parsed method: '" << request.getMethodStr() << "'" << std::endl;

    // Test validation
    ValidationResult validation_result = request.validateRequest();
    std::cout << "validateRequest result: " << validation_result << std::endl;

    if (validation_result == INVALID_METHOD) {
        std::cout << "✅ Correctly identified as INVALID_METHOD (405)" << std::endl;
    } else {
        std::cout << "❌ Wrong validation result: " << validation_result << std::endl;
    }
}

int main() {
    std::cout << "Large Request Unit Tests" << std::endl;
    std::cout << "========================" << std::endl;

    test_invalid_method_parsing();
    test_large_uri_parsing();
    test_large_header_parsing();

    std::cout << "\n=== Test Complete ===" << std::endl;
    return 0;
}