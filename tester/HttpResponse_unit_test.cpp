#include "../http/http_response.hpp"
#include <cassert>

// void test_resultToStatusCode() {
//     HttpResponse response;

//     // Test 400 series errors
//     response.resultToStatusCode(BAD_REQUEST);
//     assert(response.getStatusCode() == 400);

//     response.resultToStatusCode(NOT_FOUND);
//     assert(response.getStatusCode() == 404);

//     response.resultToStatusCode(INVALID_METHOD);
//     assert(response.getStatusCode() == 405);

//     // Test 200 series success
//     response.resultToStatusCode(VALID_REQUEST);
//     assert(response.getStatusCode() == 200);

//     response.resultToStatusCode(CREATED);
//     assert(response.getStatusCode() == 201);

//     // Test 500 series errors
//     response.resultToStatusCode(INTERNAL_SERVER_ERROR);
//     assert(response.getStatusCode() == 500);

//     std::cout << "✅ All resultToStatusCode tests passed!" << std::endl;
// }

// void test_responseStatusLine() {
//     HttpResponse response;
    
//     // Test 200 series
//     response.resultToStatusCode(VALID_REQUEST);
//     assert(response.responseStatusLine() == "HTTP/1.1 200 OK\r\n");
    
//     response.resultToStatusCode(CREATED);
//     assert(response.responseStatusLine() == "HTTP/1.1 201 Created\r\n");
    
//     response.resultToStatusCode(NO_CONTENT);
//     assert(response.responseStatusLine() == "HTTP/1.1 204 No Content\r\n");
    
//     // Test 300 series
//     response.resultToStatusCode(MOVED_PERMANENTLY);
//     assert(response.responseStatusLine() == "HTTP/1.1 301 Moved Permanently\r\n");
    
//     response.resultToStatusCode(FOUND);
//     assert(response.responseStatusLine() == "HTTP/1.1 302 Found\r\n");
    
//     // Test 400 series - common client errors
//     response.resultToStatusCode(BAD_REQUEST);
//     assert(response.responseStatusLine() == "HTTP/1.1 400 Bad Request\r\n");
    
//     response.resultToStatusCode(UNAUTORIZED);
//     assert(response.responseStatusLine() == "HTTP/1.1 401 Unauthorized\r\n");
    
//     response.resultToStatusCode(FORBIDDEN);
//     assert(response.responseStatusLine() == "HTTP/1.1 403 Forbidden\r\n");
    
//     response.resultToStatusCode(NOT_FOUND);
//     assert(response.responseStatusLine() == "HTTP/1.1 404 Not Found\r\n");
    
//     response.resultToStatusCode(INVALID_METHOD);
//     assert(response.responseStatusLine() == "HTTP/1.1 405 Method Not Allowed\r\n");
    
//     // Test 400 series - validation errors that map to 400
//     response.resultToStatusCode(INVALID_REQUEST_LINE);
//     assert(response.responseStatusLine() == "HTTP/1.1 400 Bad Request\r\n");
    
//     response.resultToStatusCode(INVALID_URI);
//     assert(response.responseStatusLine() == "HTTP/1.1 400 Bad Request\r\n");
    
//     response.resultToStatusCode(MISSING_HOST_HEADER);
//     assert(response.responseStatusLine() == "HTTP/1.1 400 Bad Request\r\n");
    
//     // Test specific 400 series codes
//     response.resultToStatusCode(REQUEST_TIMEOUT);
//     assert(response.responseStatusLine() == "HTTP/1.1 408 Request Timeout\r\n");
    
//     response.resultToStatusCode(LENGTH_REQUIRED);
//     assert(response.responseStatusLine() == "HTTP/1.1 411 Length Required\r\n");
    
//     response.resultToStatusCode(PAYLOAD_TOO_LARGE);
//     assert(response.responseStatusLine() == "HTTP/1.1 413 Payload Too Large\r\n");
    
//     response.resultToStatusCode(URI_TOO_LONG);
//     assert(response.responseStatusLine() == "HTTP/1.1 414 URI Too Long\r\n");
    
//     response.resultToStatusCode(HEADER_TOO_LARGE);
//     assert(response.responseStatusLine() == "HTTP/1.1 431 Request Header Fields Too Large\r\n");
    
//     // Test 500 series
//     response.resultToStatusCode(INTERNAL_SERVER_ERROR);
//     assert(response.responseStatusLine() == "HTTP/1.1 500 Internal Server Error\r\n");
    
//     response.resultToStatusCode(NOT_IMPLEMENTED);
//     assert(response.responseStatusLine() == "HTTP/1.1 501 Not Implemented\r\n");
    
//     response.resultToStatusCode(BAD_GATEWAY);
//     assert(response.responseStatusLine() == "HTTP/1.1 502 Bad Gateway\r\n");
    
//     response.resultToStatusCode(SERVICE_UNAVAILABLE);
//     assert(response.responseStatusLine() == "HTTP/1.1 503 Service Unavailable\r\n");
    
//     response.resultToStatusCode(GATEWAY_TIMEOUT);
//     assert(response.responseStatusLine() == "HTTP/1.1 504 Gateway Timeout\r\n");
    
//     std::cout << "✅ All responseStatusLine tests passed!" << std::endl;
// }

// void test_responseHeader()
// {
//     HttpRequest request;
//     HttpResponse response;
//     // manuall set connection as keep-alive for testing
//     request.setConnection(true);
//     // manuall set body info for testing
//     response.setBody("<html><body>Hello World</body></html>");

//     response.setRequiredHeader(request);
//     std::string header = response.responseHeader();
//     std::cout << "header output: \n" << header << std::endl;


// }

void test_buildFullResponse()
{
    HttpRequest request;
    HttpResponse response;

    request.setValidationResult(BAD_REQUEST);
    request.setConnection(false);
    std::string fullResponse = response.buildFullResponse(request);
    std::cout << fullResponse << std::endl;
}

void test_buildFileResponse()
{
    HttpResponse response;
    std::string file_path = "../www/nonexist.html";

    std::string result = response.buildFileResponse(file_path);
    std::cout << "Output: " << result << std::endl;
}

int main() {
    // test_resultToStatusCode();
    // test_responseStatusLine();
    // test_responseHeader();
    // test_buildFullResponse();
    test_buildFileResponse();
    
    return 0;
}