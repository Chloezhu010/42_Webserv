#include "../http/http_request.hpp"
#include <cassert>
#include <iostream>

// Test basic multipart detection
void test_isMultipartFormData() {
    std::cout << "Testing isMultipartFormData()..." << std::endl;

    HttpRequest request;

    // Test 1: Valid multipart content type
    std::string valid_multipart = "POST /upload HTTP/1.1\r\n"
                                 "Host: localhost\r\n"
                                 "Content-Type: multipart/form-data; boundary=----WebKitFormBoundary7MA4YWxkTrZu0gW\r\n"
                                 "Content-Length: 0\r\n"
                                 "\r\n";
    request.parseRequest(valid_multipart);

    assert(request.isMultipartFormData() == true);
    std::cout << "✅ Valid multipart detection passed" << std::endl;

    // Test 2: Non-multipart content type
    HttpRequest request2;
    std::string non_multipart = "POST /upload HTTP/1.1\r\n"
                               "Host: localhost\r\n"
                               "Content-Type: application/json\r\n"
                               "Content-Length: 0\r\n"
                               "\r\n";
    request2.parseRequest(non_multipart);

    assert(request2.isMultipartFormData() == false);
    std::cout << "✅ Non-multipart detection passed" << std::endl;

    // Test 3: Missing content type
    HttpRequest request3;
    std::string no_content_type = "POST /upload HTTP/1.1\r\n"
                                 "Host: localhost\r\n"
                                 "Content-Length: 0\r\n"
                                 "\r\n";
    request3.parseRequest(no_content_type);

    assert(request3.isMultipartFormData() == false);
    std::cout << "✅ Missing content type detection passed" << std::endl;
}

// Test simple form field parsing
void test_simpleFormField() {
    std::cout << "\nTesting simple form field parsing..." << std::endl;

    HttpRequest request;
    std::string multipart_request =
        "POST /upload HTTP/1.1\r\n"
        "Host: localhost\r\n"
        "Content-Type: multipart/form-data; boundary=----WebKitFormBoundary123\r\n"
        "Content-Length: 121\r\n"
        "\r\n"
        "------WebKitFormBoundary123\r\n"
        "Content-Disposition: form-data; name=\"username\"\r\n"
        "\r\n"
        "john_doe\r\n"
        "------WebKitFormBoundary123--\r\n";

    // Calculate actual body length
    size_t header_end = multipart_request.find("\r\n\r\n");
    std::string body_part = multipart_request.substr(header_end + 4);
    std::cout << "Actual body length: " << body_part.length() << std::endl;

    bool parsed = request.parseRequest(multipart_request);
    // std::cout << "parsed request: " << parsed << std::endl;
    assert(parsed == true);

    bool multipart_parsed = request.parseMultipartFormData();
    // std::cout << "multipart parsed: " << parsed << std::endl;
    assert(multipart_parsed == true);

    std::map<std::string, std::string> form_data = request.getFormData();
    // std::cout << "form data size: " << form_data.size() << std::endl;
    assert(form_data.size() == 1);
    assert(form_data["username"] == "john_doe");

    std::cout << "✅ Simple form field parsing passed" << std::endl;
}

// Test file upload parsing
void test_fileUploadParsing() {
    std::cout << "\nTesting file upload parsing..." << std::endl;

    HttpRequest request;
    std::string file_content = "Hello World!\nThis is test file content.";
    std::string multipart_request =
        "POST /upload HTTP/1.1\r\n"
        "Host: localhost\r\n"
        "Content-Type: multipart/form-data; boundary=----WebKitFormBoundary456\r\n"
        "Content-Length: 195\r\n"
        "\r\n"
        "------WebKitFormBoundary456\r\n"
        "Content-Disposition: form-data; name=\"file\"; filename=\"test.txt\"\r\n"
        "Content-Type: text/plain\r\n"
        "\r\n" +
        file_content + "\r\n"
        "------WebKitFormBoundary456--\r\n";

    // Calculate actual body length
    size_t header_end = multipart_request.find("\r\n\r\n");
    std::string body_part = multipart_request.substr(header_end + 4);
    std::cout << "Actual body length: " << body_part.length() << std::endl;

    bool parsed = request.parseRequest(multipart_request);
    assert(parsed == true);

    bool multipart_parsed = request.parseMultipartFormData();
    assert(multipart_parsed == true);

    std::vector<FileUpload> files = request.getUploadedFiles();
    assert(files.size() == 1);
    assert(files[0].name == "file");
    assert(files[0].filename == "test.txt");
    assert(files[0].content == file_content);
    assert(files[0].size == file_content.length());
    // Content type should be determined by extension, not header
    assert(files[0].content_type == "text/plain; charset=UTF-8");

    std::cout << "✅ File upload parsing passed" << std::endl;
}

// Test mixed form fields and file uploads
void test_mixedFormData() {
    std::cout << "\nTesting mixed form data..." << std::endl;

    HttpRequest request;
    std::string file_content = "Binary image data here";
    std::string multipart_request =
        "POST /upload HTTP/1.1\r\n"
        "Host: localhost\r\n"
        "Content-Type: multipart/form-data; boundary=----WebKitFormBoundary789\r\n"
        "Content-Length: 371\r\n"
        "\r\n"
        "------WebKitFormBoundary789\r\n"
        "Content-Disposition: form-data; name=\"username\"\r\n"
        "\r\n"
        "alice\r\n"
        "------WebKitFormBoundary789\r\n"
        "Content-Disposition: form-data; name=\"avatar\"; filename=\"photo.jpg\"\r\n"
        "Content-Type: image/jpeg\r\n"
        "\r\n" +
        file_content + "\r\n"
        "------WebKitFormBoundary789\r\n"
        "Content-Disposition: form-data; name=\"description\"\r\n"
        "\r\n"
        "User profile photo\r\n"
        "------WebKitFormBoundary789--\r\n";
    
    // Calculate actual body length
    size_t header_end = multipart_request.find("\r\n\r\n");
    std::string body_part = multipart_request.substr(header_end + 4);
    std::cout << "Actual body length: " << body_part.length() << std::endl;

    bool parsed = request.parseRequest(multipart_request);
    assert(parsed == true);

    bool multipart_parsed = request.parseMultipartFormData();
    assert(multipart_parsed == true);

    // Check form fields
    std::map<std::string, std::string> form_data = request.getFormData();
    assert(form_data.size() == 2);
    assert(form_data["username"] == "alice");
    assert(form_data["description"] == "User profile photo");

    // Check file upload
    std::vector<FileUpload> files = request.getUploadedFiles();
    assert(files.size() == 1);
    assert(files[0].name == "avatar");
    assert(files[0].filename == "photo.jpg");
    assert(files[0].content == file_content);
    assert(files[0].content_type == "image/jpeg");

    std::cout << "✅ Mixed form data parsing passed" << std::endl;
}

// Test unquoted parameter values
void test_unquotedParameters() {
    std::cout << "\nTesting unquoted parameter values..." << std::endl;

    HttpRequest request;
    std::string multipart_request =
        "POST /upload HTTP/1.1\r\n"
        "Host: localhost\r\n"
        "Content-Type: multipart/form-data; boundary=----WebKitFormBoundaryABC\r\n"
        "Content-Length: 231\r\n"
        "\r\n"
        "------WebKitFormBoundaryABC\r\n"
        "Content-Disposition: form-data; name=field1\r\n"
        "\r\n"
        "value1\r\n"
        "------WebKitFormBoundaryABC\r\n"
        "Content-Disposition: form-data; name=file1; filename=document.pdf\r\n"
        "\r\n"
        "PDF content here\r\n"
        "------WebKitFormBoundaryABC--\r\n";
    
    // Calculate actual body length
    size_t header_end = multipart_request.find("\r\n\r\n");
    std::string body_part = multipart_request.substr(header_end + 4);
    std::cout << "Actual body length: " << body_part.length() << std::endl;

    bool parsed = request.parseRequest(multipart_request);
    assert(parsed == true);

    bool multipart_parsed = request.parseMultipartFormData();
    assert(multipart_parsed == true);

    // Check form field with unquoted name
    std::map<std::string, std::string> form_data = request.getFormData();
    assert(form_data.size() == 1);
    assert(form_data["field1"] == "value1");

    // Check file with unquoted parameters
    std::vector<FileUpload> files = request.getUploadedFiles();
    assert(files.size() == 1);
    assert(files[0].name == "file1");
    assert(files[0].filename == "document.pdf");
    assert(files[0].content_type == "application/pdf");

    std::cout << "✅ Unquoted parameter parsing passed" << std::endl;
}

// Test filename with spaces
void test_filenameWithSpaces() {
    std::cout << "\nTesting filename with spaces..." << std::endl;

    HttpRequest request;
    std::string multipart_request =
        "POST /upload HTTP/1.1\r\n"
        "Host: localhost\r\n"
        "Content-Type: multipart/form-data; boundary=----WebKitFormBoundaryDEF\r\n"
        "Content-Length: 164\r\n"
        "\r\n"
        "------WebKitFormBoundaryDEF\r\n"
        "Content-Disposition: form-data; name=\"document\"; filename=\"my important file.docx\"\r\n"
        "\r\n"
        "Document content\r\n"
        "------WebKitFormBoundaryDEF--\r\n";

    // Calculate actual body length
    size_t header_end = multipart_request.find("\r\n\r\n");
    std::string body_part = multipart_request.substr(header_end + 4);
    std::cout << "Actual body length: " << body_part.length() << std::endl;
    bool parsed = request.parseRequest(multipart_request);
    assert(parsed == true);

    bool multipart_parsed = request.parseMultipartFormData();
    assert(multipart_parsed == true);

    std::vector<FileUpload> files = request.getUploadedFiles();
    assert(files.size() == 1);
    assert(files[0].filename == "my important file.docx");
    assert(files[0].content_type == "application/vnd.openxmlformats-officedocument.wordprocessingml.document");

    std::cout << "✅ Filename with spaces parsing passed" << std::endl;
}

// Test multiple files upload
void test_multipleFiles() {
    std::cout << "\nTesting multiple file uploads..." << std::endl;

    HttpRequest request;
    std::string multipart_request =
        "POST /upload HTTP/1.1\r\n"
        "Host: localhost\r\n"
        "Content-Type: multipart/form-data; boundary=----WebKitFormBoundaryGHI\r\n"
        "Content-length: 385\r\n"
        "\r\n"
        "------WebKitFormBoundaryGHI\r\n"
        "Content-Disposition: form-data; name=\"file1\"; filename=\"image.png\"\r\n"
        "\r\n"
        "PNG image data\r\n"
        "------WebKitFormBoundaryGHI\r\n"
        "Content-Disposition: form-data; name=\"file2\"; filename=\"script.js\"\r\n"
        "\r\n"
        "console.log('Hello');\r\n"
        "------WebKitFormBoundaryGHI\r\n"
        "Content-Disposition: form-data; name=\"file3\"; filename=\"data.json\"\r\n"
        "\r\n"
        "{\"key\": \"value\"}\r\n"
        "------WebKitFormBoundaryGHI--\r\n";
    
    // Calculate actual body length
    size_t header_end = multipart_request.find("\r\n\r\n");
    std::string body_part = multipart_request.substr(header_end + 4);
    std::cout << "Actual body length: " << body_part.length() << std::endl;

    bool parsed = request.parseRequest(multipart_request);
    assert(parsed == true);

    bool multipart_parsed = request.parseMultipartFormData();
    assert(multipart_parsed == true);

    std::vector<FileUpload> files = request.getUploadedFiles();
    std::cout << "file size: " << files.size() << std::endl;
    assert(files.size() == 3);

    // Check each file
    assert(files[0].filename == "image.png");
    assert(files[0].content_type == "image/png");

    assert(files[1].filename == "script.js");
    assert(files[1].content_type == "application/javascript; charset=UTF-8");

    assert(files[2].filename == "data.json");
    assert(files[2].content_type == "application/json; charset=UTF-8");

    std::cout << "✅ Multiple file uploads parsing passed" << std::endl;
}

// Test malformed multipart data
void test_malformedData() {
    std::cout << "\nTesting malformed multipart data..." << std::endl;

    // Test 1: Missing boundary
    HttpRequest request1;
    std::string no_boundary =
        "POST /upload HTTP/1.1\r\n"
        "Host: localhost\r\n"
        "Content-Type: multipart/form-data\r\n"
        "Content-length: 114\r\n"
        "\r\n"
        "some content without boundary\r\n";

    // Calculate actual body length
    size_t header_end = no_boundary.find("\r\n\r\n");
    std::string body_part = no_boundary.substr(header_end + 4);
    std::cout << "Actual body length: " << body_part.length() << std::endl;
    
    request1.parseRequest(no_boundary);
    bool result1 = request1.parseMultipartFormData();
    assert(result1 == false);
    std::cout << "✅ Missing boundary handling passed" << std::endl;

    // Test 2: Missing name parameter
    HttpRequest request2;
    std::string no_name =
        "POST /upload HTTP/1.1\r\n"
        "Host: localhost\r\n"
        "Content-Type: multipart/form-data; boundary=----WebKitFormBoundaryBAD\r\n"
        "Content-length: 200\r\n"
        "\r\n"
        "------WebKitFormBoundaryBAD\r\n"
        "Content-Disposition: form-data\r\n"
        "\r\n"
        "value without name\r\n"
        "------WebKitFormBoundaryBAD--\r\n";

    // Calculate actual body length
    size_t header_end2 = no_name.find("\r\n\r\n");
    std::string body_part2 = no_name.substr(header_end2 + 4);
    std::cout << "Actual body length: " << body_part2.length() << std::endl;

    request2.parseRequest(no_name);
    bool result2 = request2.parseMultipartFormData();
    assert(result2 == false);
    std::cout << "✅ Missing name parameter handling passed" << std::endl;

    // Test 3: Malformed headers (no double CRLF)
    HttpRequest request3;
    std::string bad_headers =
        "POST /upload HTTP/1.1\r\n"
        "Host: localhost\r\n"
        "Content-Type: multipart/form-data; boundary=----WebKitFormBoundaryBAD2\r\n"
        "Content-length: 139\r\n"
        "\r\n"
        "------WebKitFormBoundaryBAD2\r\n"
        "Content-Disposition: form-data; name=\"field\"\r\n"
        "missing double CRLF separator\r\n"
        "------WebKitFormBoundaryBAD2--\r\n";

    // Calculate actual body length
    size_t header_end3 = bad_headers.find("\r\n\r\n");
    std::string body_part3 = bad_headers.substr(header_end3 + 4);
    std::cout << "Actual body length: " << body_part3.length() << std::endl;

    request3.parseRequest(bad_headers);
    bool result3 = request3.parseMultipartFormData();
    assert(result3 == false);
    std::cout << "✅ Malformed headers handling passed" << std::endl;
}

// Test edge case: empty filename
void test_emptyFilename() {
    std::cout << "\nTesting empty filename..." << std::endl;

    HttpRequest request;
    std::string empty_filename =
        "POST /upload HTTP/1.1\r\n"
        "Host: localhost\r\n"
        "Content-Type: multipart/form-data; boundary=----WebKitFormBoundaryEMPTY\r\n"
        "Content-length: 150\r\n"
        "\r\n"
        "------WebKitFormBoundaryEMPTY\r\n"
        "Content-Disposition: form-data; name=\"file\"; filename=\"\"\r\n"
        "\r\n"
        "content without filename\r\n"
        "------WebKitFormBoundaryEMPTY--\r\n";

    // Calculate actual body length
    size_t header_end = empty_filename.find("\r\n\r\n");
    std::string body_part = empty_filename.substr(header_end + 4);
    std::cout << "Actual body length: " << body_part.length() << std::endl;

    bool parsed = request.parseRequest(empty_filename);
    assert(parsed == true);

    bool multipart_parsed = request.parseMultipartFormData();
    assert(multipart_parsed == true);

    // Should be treated as form field, not file upload
    std::map<std::string, std::string> form_data = request.getFormData();
    assert(form_data.size() == 1);
    assert(form_data["file"] == "content without filename");

    std::vector<FileUpload> files = request.getUploadedFiles();
    assert(files.size() == 0);

    std::cout << "✅ Empty filename handling passed" << std::endl;
}

// Test MIME type detection by extension
void test_mimeTypeDetection() {
    std::cout << "\nTesting MIME type detection..." << std::endl;

    struct TestCase {
        std::string filename;
        std::string expected_mime;
    };

    TestCase test_cases[] = {
        {"document.pdf", "application/pdf"},
        {"image.PNG", "image/png"},  // Test case insensitive
        {"archive.zip", "application/zip"},
        {"video.mp4", "video/mp4"},
        {"audio.mp3", "audio/mpeg"},
        {"stylesheet.css", "text/css; charset=UTF-8"},
        {"unknownfile.xyz", "application/octet-stream"},
        {"noextension", "application/octet-stream"}
    };

    for (size_t i = 0; i < sizeof(test_cases) / sizeof(test_cases[0]); ++i) {
        HttpRequest request;
        std::string multipart_request =
            "POST /upload HTTP/1.1\r\n"
            "Host: localhost\r\n"
            "Content-Type: multipart/form-data; boundary=----WebKitFormBoundaryMIME\r\n"
            "Content-length: 148\r\n"
                "\r\n"
            "------WebKitFormBoundaryMIME\r\n"
            "Content-Disposition: form-data; name=\"file\"; filename=\"" + test_cases[i].filename + "\"\r\n"
            "\r\n"
            "file content\r\n"
            "------WebKitFormBoundaryMIME--\r\n";

        // Calculate actual body length
        size_t header_end = multipart_request.find("\r\n\r\n");
        std::string body_part = multipart_request.substr(header_end + 4);
        std::cout << "Actual body length for " << test_cases[i].filename << ": " << body_part.length() << std::endl;

        // Update Content-Length dynamically
        std::ostringstream length_stream;
        length_stream << body_part.length();
        std::string dynamic_length = length_stream.str();

        // Replace the hardcoded Content-Length with the actual length
        size_t cl_pos = multipart_request.find("Content-length: ");
        if (cl_pos != std::string::npos) {
            size_t cl_end = multipart_request.find("\r\n", cl_pos);
            if (cl_end != std::string::npos) {
                std::string old_line = multipart_request.substr(cl_pos, cl_end - cl_pos);
                multipart_request.replace(cl_pos, old_line.length(), "Content-length: " + dynamic_length);
            }
        }

        request.parseRequest(multipart_request);
        request.parseMultipartFormData();

        std::vector<FileUpload> files = request.getUploadedFiles();
        assert(files.size() == 1);
        assert(files[0].content_type == test_cases[i].expected_mime);

        std::cout << "✅ " << test_cases[i].filename << " → " << test_cases[i].expected_mime << std::endl;
    }
}

int main() {
    std::cout << "=== Multipart Form Data Parsing Tests ===\n" << std::endl;

    try {
        test_isMultipartFormData();
        test_simpleFormField();
        test_fileUploadParsing();
        test_mixedFormData();
        test_unquotedParameters();
        test_filenameWithSpaces();
        test_multipleFiles();
        test_malformedData();
        test_emptyFilename();
        test_mimeTypeDetection();

        std::cout << "\n🎉 All multipart form data tests passed!" << std::endl;
    } catch (const std::exception& e) {
        std::cout << "\n❌ Test failed: " << e.what() << std::endl;
        return 1;
    }

    return 0;
}