#include "../http/http_request.hpp"
#include <iostream>
#include <cassert>

// Test result tracking
struct TestResult {
    int total;
    int passed;
    int failed;

    TestResult() : total(0), passed(0), failed(0) {}

    void addResult(bool success) {
        total++;
        if (success) passed++;
        else failed++;
    }

    void printSummary() {
        std::cout << "\n=== TEST SUMMARY ===" << std::endl;
        std::cout << "Total tests: " << total << std::endl;
        std::cout << "Passed: " << passed << std::endl;
        std::cout << "Failed: " << failed << std::endl;
        std::cout << "Success rate: " << (passed * 100 / total) << "%" << std::endl;
    }
};

TestResult g_results;

void testValidationResult(const std::string& test_name, const std::string& request,
                         ValidationResult expected_result) {
    std::cout << "\n--- " << test_name << " ---" << std::endl;

    HttpRequest req;
    bool parse_success = req.parseRequest(request);

    if (!parse_success) {
        std::cout << "❌ Parse failed (expected validation to handle it)" << std::endl;
        g_results.addResult(false);
        return;
    }

    ValidationResult actual_result = req.validateRequest();

    if (actual_result == expected_result) {
        std::cout << "✅ PASS: Got " << actual_result << " as expected" << std::endl;
        g_results.addResult(true);
    } else {
        std::cout << "❌ FAIL: Expected " << expected_result << ", got " << actual_result << std::endl;
        g_results.addResult(false);
    }
}

void test_2xx_success() {
    std::cout << "\n======= 2xx SUCCESS RESPONSES =======" << std::endl;

    // VALID_REQUEST (200)
    testValidationResult(
        "Valid GET request",
        "GET / HTTP/1.1\r\nHost: localhost\r\n\r\n",
        VALID_REQUEST
    );

    testValidationResult(
        "Valid POST request",
        "POST /submit HTTP/1.1\r\nHost: localhost\r\nContent-Length: 5\r\n\r\nhello",
        VALID_REQUEST
    );

    testValidationResult(
        "Valid DELETE request",
        "DELETE /item HTTP/1.1\r\nHost: localhost\r\n\r\n",
        VALID_REQUEST
    );
}

void test_4xx_client_errors() {
    std::cout << "\n======= 4xx CLIENT ERROR RESPONSES =======" << std::endl;

    // BAD_REQUEST (400) - Various malformed requests
    testValidationResult(
        "Content-Length mismatch",
        "POST /submit HTTP/1.1\r\nHost: localhost\r\nContent-Length: 10\r\n\r\nhello",
        BAD_REQUEST
    );

    // // INVALID_REQUEST_LINE (400)
    // testValidationResult(
    //     "Empty method/URI/version",
    //     "GET  HTTP/1.1\r\nHost: localhost\r\n\r\n",  // Missing URI
    //     INVALID_REQUEST_LINE
    // );

    // INVALID_HTTP_VERSION (400)
    testValidationResult(
        "Invalid HTTP version",
        "GET / HTTP/1.0\r\nHost: localhost\r\n\r\n",
        INVALID_HTTP_VERSION
    );

    testValidationResult(
        "Wrong HTTP version format",
        "GET / INVALID_VERSION\r\nHost: localhost\r\n\r\n",
        INVALID_HTTP_VERSION
    );

    // INVALID_URI (400)
    testValidationResult(
        "Path traversal attack",
        "GET /../etc/passwd HTTP/1.1\r\nHost: localhost\r\n\r\n",
        INVALID_URI
    );

    testValidationResult(
        "URI with null byte",
        std::string("GET /test\0/file HTTP/1.1\r\nHost: localhost\r\n\r\n", 48),
        INVALID_URI
    );

    testValidationResult(
        "URI with control characters",
        "GET /test\x01/file HTTP/1.1\r\nHost: localhost\r\n\r\n",
        INVALID_URI
    );

    // INVALID_HEADER (400)
    testValidationResult(
        "Missing Host header",
        "GET / HTTP/1.1\r\n\r\n",
        INVALID_HEADER
    );

    testValidationResult(
        "Empty Host header",
        "GET / HTTP/1.1\r\nHost: \r\n\r\n",
        INVALID_HEADER
    );

    testValidationResult(
        "Multiple Host headers",
        "GET / HTTP/1.1\r\nHost: localhost\r\nHost: example.com\r\n\r\n",
        INVALID_HEADER
    );

    // INVALID_CONTENT_LENGTH (400)
    testValidationResult(
        "Invalid Content-Length format",
        "POST /submit HTTP/1.1\r\nHost: localhost\r\nContent-Length: abc\r\n\r\n",
        INVALID_HEADER
    );

    testValidationResult(
        "Negative Content-Length",
        "POST /submit HTTP/1.1\r\nHost: localhost\r\nContent-Length: -10\r\n\r\n",
        INVALID_HEADER
    );

    // CONFLICTING_HEADER (400)
    testValidationResult(
        "Transfer-Encoding and Content-Length conflict",
        "POST /submit HTTP/1.1\r\nHost: localhost\r\nContent-Length: 5\r\nTransfer-Encoding: chunked\r\n\r\nhello",
        CONFLICTING_HEADER
    );

    // METHOD_BODY_MISMATCH (400)
    testValidationResult(
        "GET with Content-Length",
        "GET / HTTP/1.1\r\nHost: localhost\r\nContent-Length: 5\r\n\r\nhello",
        METHOD_BODY_MISMATCH
    );

    // MISSING_HOST_HEADER (400) - should be caught by INVALID_HEADER

    // INVALID_METHOD (405)
    testValidationResult(
        "PATCH method not allowed",
        "PATCH / HTTP/1.1\r\nHost: localhost\r\n\r\n",
        INVALID_METHOD
    );

    testValidationResult(
        "Custom invalid method",
        "CUSTOM_METHOD / HTTP/1.1\r\nHost: localhost\r\n\r\n",
        INVALID_METHOD
    );

    // LENGTH_REQUIRED (411)
    testValidationResult(
        "POST without Content-Length",
        "POST /submit HTTP/1.1\r\nHost: localhost\r\n\r\nhello",
        LENGTH_REQUIRED
    );

    // PAYLOAD_TOO_LARGE (413) - Body size
    std::string large_body(11 * 1024 * 1024, 'x');  // 11MB body
    std::ostringstream large_post;
    large_post << "POST /submit HTTP/1.1\r\nHost: localhost\r\nContent-Length: "
               << large_body.length() << "\r\n\r\n" << large_body;
    testValidationResult(
        "Body size exceeds limit",
        large_post.str(),
        PAYLOAD_TOO_LARGE
    );

    // URI_TOO_LONG (414)
    std::string long_uri(3000, 'a');
    testValidationResult(
        "URI exceeds length limit",
        "GET /" + long_uri + " HTTP/1.1\r\nHost: localhost\r\n\r\n",
        URI_TOO_LONG
    );

    // HEADER_TOO_LARGE (431) - Individual header too large
    std::string large_header_value(10000, 'x');  // 10KB header
    testValidationResult(
        "Single header exceeds size limit",
        "GET / HTTP/1.1\r\nHost: localhost\r\nLarge-Header: " + large_header_value + "\r\n\r\n",
        HEADER_TOO_LARGE
    );

    // HEADER_TOO_LARGE (431) - Too many headers
    std::ostringstream many_headers;
    many_headers << "GET / HTTP/1.1\r\nHost: localhost\r\n";
    for (int i = 1; i <= 150; i++) {  // Exceed MAX_HEADER_COUNT (100)
        many_headers << "Header" << i << ": value" << i << "\r\n";
    }
    many_headers << "\r\n";
    testValidationResult(
        "Too many headers",
        many_headers.str(),
        HEADER_TOO_LARGE
    );
}

void test_total_request_size_limit() {
    std::cout << "\n======= TOTAL REQUEST SIZE TESTS =======" << std::endl;

    // PAYLOAD_TOO_LARGE (413) - Total request size
    std::string huge_header(9 * 1024 * 1024, 'x');  // 9MB header to exceed total size
    testValidationResult(
        "Total request size exceeds 8MB limit",
        "GET / HTTP/1.1\r\nHost: localhost\r\nHuge-Header: " + huge_header + "\r\n\r\n",
        PAYLOAD_TOO_LARGE
    );
}

void test_edge_cases() {
    std::cout << "\n======= EDGE CASES =======" << std::endl;

    // Zero-length POST body
    testValidationResult(
        "POST with zero Content-Length",
        "POST /submit HTTP/1.1\r\nHost: localhost\r\nContent-Length: 0\r\n\r\n",
        VALID_REQUEST
    );

    // Exactly at URI limit (should pass)
    std::string uri_at_limit(2048, 'a');
    testValidationResult(
        "URI exactly at limit (2048 chars)",
        "GET /" + uri_at_limit + " HTTP/1.1\r\nHost: localhost\r\n\r\n",
        VALID_REQUEST
    );

    // URI just over limit (should fail)
    std::string uri_over_limit(2049, 'a');
    testValidationResult(
        "URI just over limit (2049 chars)",
        "GET /" + uri_over_limit + " HTTP/1.1\r\nHost: localhost\r\n\r\n",
        URI_TOO_LONG
    );

    // Valid chunked encoding (if supported)
    testValidationResult(
        "Valid chunked encoding",
        "POST /submit HTTP/1.1\r\nHost: localhost\r\nTransfer-Encoding: chunked\r\n\r\n5\r\nhello\r\n0\r\n\r\n",
        VALID_REQUEST
    );

    // Case-insensitive host header
    testValidationResult(
        "Case-insensitive Host header",
        "GET / HTTP/1.1\r\nHOST: localhost\r\n\r\n",
        VALID_REQUEST
    );
}

void test_parsing_robustness() {
    std::cout << "\n======= PARSING ROBUSTNESS =======" << std::endl;

    // Test that parser handles large requests without crashing
    std::string large_but_valid_request = "GET / HTTP/1.1\r\nHost: localhost\r\n";
    for (int i = 0; i < 50; i++) {
        large_but_valid_request += "Header" + std::to_string(i) + ": value\r\n";
    }
    large_but_valid_request += "\r\n";

    testValidationResult(
        "Large but valid request",
        large_but_valid_request,
        VALID_REQUEST
    );
}

int main() {
    std::cout << "COMPREHENSIVE HTTP REQUEST VALIDATION TEST SUITE" << std::endl;
    std::cout << "=================================================" << std::endl;

    // Run all test categories
    test_2xx_success();
    test_4xx_client_errors();
    test_total_request_size_limit();
    test_edge_cases();
    test_parsing_robustness();

    // Print final results
    g_results.printSummary();

    if (g_results.failed == 0) {
        std::cout << "\n🎉 ALL TESTS PASSED!" << std::endl;
        return 0;
    } else {
        std::cout << "\n⚠️  " << g_results.failed << " TESTS FAILED" << std::endl;
        return 1;
    }
}