#include <iostream>
#include <string>
#include <fstream>
#include <sstream>

int main()
{
    // // test extract url path from http request
    // std::string url = "GET /about.html HTTP/1.1";
    // size_t first_space = url.find(' ');
    // size_t second_space = url.find(' ', first_space + 1);
    // std::cout << "first_space: " << first_space << std::endl;
    // std::cout << "second_space: " << second_space << std::endl;
    // std::cout << "url: " << url.substr(first_space + 1, second_space - first_space - 1) << std::endl;
    
    // // test read file
    // std::string file_name = "about.html";
    // std::ifstream file(file_name.c_str());
    // if (!file.is_open())
    //     return 1;
    // // std::string content;
    // // while (file >> content)
    // // {
    // //     std::cout << content << " ";
    // // }
    // std::stringstream buffer;
    // buffer << file.rdbuf();
    // std::cout << buffer.str() << std::endl;
    // file.close();

    // test generate response
    int status_code = 200;
    std::string status_text;
    switch (status_code) {
        case 200: status_text = "OK"; break;
        case 404: status_text = "Not Found"; break;
        default: status_text = "Internal Server Error"; break;
    }
    std::stringstream response;
    response << "HTTP/1.1 " << status_code << " " << status_text << "\n";
    response << "Content-Type: text/html; charset=UTF-8\n";
    response << "Server: webserv/2.0-nonblocking\n";
    response << "Connection: close\n";  // TODO, need to update to other options
    response << "\n";
    std::cout << response.str() << std::endl;

    return 0;
}