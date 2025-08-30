#include <string>
#include <vector>
#include <iostream>
#include <cassert>
#include "../http/http_request.hpp"

// ============================================================================
// TEST DATA STRUCTURE
// ============================================================================

struct CompletenessTestCase {
    std::string name;
    std::string data;
    RequestStatus expected_status;
    std::string description;
    
    CompletenessTestCase(const std::string& n, const std::string& d, RequestStatus status, const std::string& desc)
        : name(n), data(d), expected_status(status), description(desc) {}
};

// ============================================================================
// TEST CASE DEFINITIONS
// ============================================================================

std::vector<CompletenessTestCase> createCompletenessTestCases() {
    std::vector<CompletenessTestCase> test_cases;
    
    // ========================================================================
    // 1. BASIC COMPLETENESS TESTS
    // ========================================================================
    
    test_cases.push_back(CompletenessTestCase(
        "incomplete_headers",
        "GET /index.html HTTP/1.1\r\n"
        "Host: example.com\r\n"
        "Content-Type: text/html",  // Missing \r\n\r\n
        NEED_MORE_DATA,
        "Incomplete headers should need more data"
    ));
    
    test_cases.push_back(CompletenessTestCase(
        "complete_get_request",
        "GET /index.html HTTP/1.1\r\n"
        "Host: example.com\r\n"
        "User-Agent: Mozilla/5.0\r\n"
        "\r\n",
        REQUEST_COMPLETE,
        "Complete GET request should be complete"
    ));
    
    test_cases.push_back(CompletenessTestCase(
        "invalid_method",
        "INVALID /index.html HTTP/1.1\r\n"
        "Host: example.com\r\n"
        "\r\n",
        INVALID_REQUEST,
        "Invalid method should return invalid request"
    ));
    
    // ========================================================================
    // 2. CONTENT-LENGTH TESTS
    // ========================================================================
    
    test_cases.push_back(CompletenessTestCase(
        "post_with_content_length_complete",
        "POST /submit HTTP/1.1\r\n"
        "Host: example.com\r\n"
        "Content-Length: 13\r\n"
        "Content-Type: application/json\r\n"
        "\r\n"
        "{\"key\":\"val\"}",  // Exactly 13 bytes
        REQUEST_COMPLETE,
        "POST with matching Content-Length should be complete"
    ));
    
    test_cases.push_back(CompletenessTestCase(
        "post_with_content_length_incomplete",
        "POST /submit HTTP/1.1\r\n"
        "Host: example.com\r\n"
        "Content-Length: 20\r\n"
        "Content-Type: application/json\r\n"
        "\r\n"
        "{\"key\":\"val\"}",  // Only 13 bytes, need 20
        NEED_MORE_DATA,
        "POST with insufficient body data should need more data"
    ));
    
    test_cases.push_back(CompletenessTestCase(
        "post_without_content_length",
        "POST /submit HTTP/1.1\r\n"
        "Host: example.com\r\n"
        "Content-Type: application/json\r\n"
        "\r\n"
        "{\"key\":\"val\"}",
        INVALID_REQUEST,
        "POST without Content-Length or Transfer-Encoding should be invalid"
    ));
    
    test_cases.push_back(CompletenessTestCase(
        "post_zero_content_length",
        "POST /submit HTTP/1.1\r\n"
        "Host: example.com\r\n"
        "Content-Length: 0\r\n"
        "\r\n",
        REQUEST_COMPLETE,
        "POST with zero Content-Length should be complete"
    ));
    
    // ========================================================================
    // 3. CHUNKED ENCODING TESTS
    // ========================================================================
    
    test_cases.push_back(CompletenessTestCase(
        "chunked_complete",
        "POST /upload HTTP/1.1\r\n"
        "Host: example.com\r\n"
        "Transfer-Encoding: chunked\r\n"
        "\r\n"
        "7\r\n"
        "Mozilla\r\n"
        "9\r\n"
        "Developer\r\n"
        "0\r\n"
        "\r\n",
        REQUEST_COMPLETE,
        "Complete chunked request should be complete"
    ));
    
    test_cases.push_back(CompletenessTestCase(
        "chunked_incomplete_no_final_chunk",
        "POST /upload HTTP/1.1\r\n"
        "Host: example.com\r\n"
        "Transfer-Encoding: chunked\r\n"
        "\r\n"
        "7\r\n"
        "Mozilla\r\n"
        "9\r\n"
        "Developer\r\n",  // Missing final 0\r\n\r\n
        NEED_MORE_DATA,
        "Chunked request without final chunk should need more data"
    ));
    
    test_cases.push_back(CompletenessTestCase(
        "chunked_incomplete_partial_chunk",
        "POST /upload HTTP/1.1\r\n"
        "Host: example.com\r\n"
        "Transfer-Encoding: chunked\r\n"
        "\r\n"
        "7\r\n"
        "Mozi",  // Incomplete chunk data
        NEED_MORE_DATA,
        "Chunked request with partial chunk should need more data"
    ));
    
    test_cases.push_back(CompletenessTestCase(
        "chunked_with_trailer",
        "POST /upload HTTP/1.1\r\n"
        "Host: example.com\r\n"
        "Transfer-Encoding: chunked\r\n"
        "\r\n"
        "7\r\n"
        "Mozilla\r\n"
        "0\r\n"
        "Expires: Wed, 21 Oct 2015 07:28:00 GMT\r\n"
        "\r\n",
        REQUEST_COMPLETE,
        "Chunked request with trailer headers should be complete"
    ));
    
    // ========================================================================
    // 4. CONFLICTING HEADERS TESTS
    // ========================================================================
    
    test_cases.push_back(CompletenessTestCase(
        "chunked_with_content_length",
        "POST /upload HTTP/1.1\r\n"
        "Host: example.com\r\n"
        "Transfer-Encoding: chunked\r\n"
        "Content-Length: 10\r\n"  // Conflicting with chunked
        "\r\n"
        "5\r\n"
        "Hello\r\n"
        "0\r\n"
        "\r\n",
        INVALID_REQUEST,
        "Request with both Transfer-Encoding and Content-Length should be invalid"
    ));
    
    // ========================================================================
    // 5. MULTIPLE TRANSFER ENCODINGS
    // ========================================================================
    
    test_cases.push_back(CompletenessTestCase(
        "multiple_encodings_chunked_last",
        "POST /upload HTTP/1.1\r\n"
        "Host: example.com\r\n"
        "Transfer-Encoding: gzip, chunked\r\n"
        "\r\n"
        "7\r\n"
        "Mozilla\r\n"
        "0\r\n"
        "\r\n",
        REQUEST_COMPLETE,
        "Multiple encodings with chunked last should be complete"
    ));
    
    test_cases.push_back(CompletenessTestCase( // will validate this case in the later phase
        "chunked_not_last",
        "POST /upload HTTP/1.1\r\n"
        "Host: example.com\r\n"
        "Transfer-Encoding: chunked, gzip\r\n"  // Invalid: chunked not last
        "\r\n"
        "7\r\n"
        "Mozilla\r\n"
        "0\r\n"
        "\r\n",
        INVALID_REQUEST,
        "Chunked encoding not last should be invalid"
    ));
    
    // ========================================================================
    // 6. EDGE CASES
    // ========================================================================
    
    test_cases.push_back(CompletenessTestCase(
        "empty_chunk_only",
        "POST /upload HTTP/1.1\r\n"
        "Host: example.com\r\n"
        "Transfer-Encoding: chunked\r\n"
        "\r\n"
        "0\r\n"
        "\r\n",
        REQUEST_COMPLETE,
        "Request with only empty chunk should be complete"
    ));
    
    test_cases.push_back(CompletenessTestCase(
        "large_chunk_size",
        "POST /upload HTTP/1.1\r\n"
        "Host: example.com\r\n"
        "Transfer-Encoding: chunked\r\n"
        "\r\n"
        "FF\r\n"  // 255 bytes
        + std::string(255, 'A') + "\r\n"
        "0\r\n"
        "\r\n",
        REQUEST_COMPLETE,
        "Request with large chunk should be complete"
    ));
    
    return test_cases;
}

// ============================================================================
// HELPER FUNCTIONS
// ============================================================================

std::string statusToString(RequestStatus status) {
    switch (status) {
        case NEED_MORE_DATA: return "NEED_MORE_DATA";
        case REQUEST_COMPLETE: return "REQUEST_COMPLETE";
        case REQUEST_TOO_LARGE: return "REQUEST_TOO_LARGE";
        case INVALID_REQUEST: return "INVALID_REQUEST";
        default: return "UNKNOWN_STATUS";
    }
}


// ============================================================================
// TEST EXECUTION FUNCTIONS
// ============================================================================

void testRequestComplete() {
    std::cout << "\n" << std::string(50, '-') << std::endl;
    std::cout << "TESTING REQUEST COMPLETENESS" << std::endl;
    std::cout << std::string(50, '-') << std::endl;
    
    std::vector<CompletenessTestCase> test_cases = createCompletenessTestCases();
    int passed = 0;
    int total = test_cases.size();
    
    for (size_t i = 0; i < test_cases.size(); i++) {
        const CompletenessTestCase& test = test_cases[i];
        std::cout << "DEBUG: Starting test " << (i + 1) << ": " << test.name << std::endl;
        std::cout << "DEBUG: " << test.description << std::endl;
        
        try {
            HttpRequest request;
            RequestStatus result = request.isRequestComplete(test.data);
            
            if (result == test.expected_status) {
                std::cout << "✅ PASS: " << test.name << std::endl;
                passed++;
            } else {
                std::cout << "❌ FAIL: " << test.name << std::endl;
                std::cout << "   Expected: " << statusToString(test.expected_status) << std::endl;
                std::cout << "   Got: " << statusToString(result) << std::endl;
                std::cout << "   Data: [" << test.data << "]" << std::endl;
            }
        } catch (const std::exception& e) {
            std::cout << "❌ EXCEPTION in test " << test.name << ": " << e.what() << std::endl;
            std::cout << "   Test data was: [" << test.data << "]" << std::endl;
        }
        
        std::cout << std::endl;
    }
    
    std::cout << "\nRequest Completeness Results: " << passed << "/" << total << " passed" << std::endl;
}


// ============================================================================
// MAIN FUNCTION
// ============================================================================

int main() {
    std::cout << std::string(60, '=') << std::endl;
    std::cout << "REQUEST COMPLETENESS TEST SUITE" << std::endl;
    std::cout << std::string(60, '=') << std::endl;

    testRequestComplete();

    return 0;
}