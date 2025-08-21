#include <iostream> // for cout, cerr
#include <sys/socket.h> // for socket functions
#include <netinet/in.h> // for sockaddr_in
#include <unistd.h> // for close
// #include <cstring>

int main()
{
    // create socket (ipv4, tcp)
    int server_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (server_fd == -1)
    {
        std::cerr << "Failed to create socket" << std::endl;
        return 1;
    }

    // set up address
    struct sockaddr_in address;
    address.sin_family = AF_INET;
    address.sin_port = htons(8080); // port 8080
    address.sin_addr.s_addr = INADDR_ANY; // bind to all interfaces

    // bind socket to address
    if (bind(server_fd, (struct sockaddr*)&address, sizeof(address)) < 0)
    {
        std::cerr << "Failed to bind socket" << std::endl;
        close(server_fd);
        return 1;
    }
    
    // listen for connections
    if (listen(server_fd, 3) < 0) // arbitrary backlog size of 3
    {
        std::cerr << "Failed to listen on socket" << std::endl;
        close(server_fd);
        return 1;
    }

    std::cout << "Server is listening on port 8080..." << std::endl;
    // accept connections
    while (true)
    {
        std::cout << "Waiting for a connection..." << std::endl;
        // accept client connection
        int connection = accept(server_fd, NULL, NULL);
        if (connection < 0)
        {
            std::cerr << "Failed to accept connection" << std::endl;
            continue; // continue to the next iteration
        }
        std::cout << "Client connected!" << std::endl;
        // echo back data
        char buffer[1024];
        while (true)
        {
            // clear buffer
            memset(buffer, 0, sizeof(buffer));
            // read data from client
            int bytes_read = read(connection, buffer, sizeof(buffer) - 1);
            if (bytes_read <= 0)
            {
                std::cerr << "Client disconnected or read error" << std::endl;
                close(connection);
                break; // exit the loop to accept a new connection
            }
            // print received data
            std::cout << "Received: " << buffer << std::endl;
            // echo data back to client
            send(connection, buffer, bytes_read, 0);
            std::cout << "Sent: " << buffer << std::endl;
        }
        close(connection);
    }

    // cleanup
    close(server_fd);
    return 0;
}