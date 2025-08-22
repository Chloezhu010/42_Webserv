#include <iostream> // for cout, cerr
#include <sys/socket.h> // for socket functions
#include <netinet/in.h> // for sockaddr_in
#include <unistd.h> // for close
#include <cstring>
#include <fcntl.h> // for fcntl()
#include <poll.h> // for poll()
#include <vector> // for vector

void printEvents(const char* label, short events)
{
    std::cout << label << ": ";
    if (events == 0) {
        std::cout << "NONE";
    } else {
        bool first = true;
        if (events & POLLIN) {
            if (!first) std::cout << " | ";
            std::cout << "POLLIN";
            first = false;
        }
        if (events & POLLOUT) {
            if (!first) std::cout << " | ";
            std::cout << "POLLOUT";
            first = false;
        }
        if (events & POLLHUP) {
            if (!first) std::cout << " | ";
            std::cout << "POLLHUP";
            first = false;
        }
        if (events & POLLERR) {
            if (!first) std::cout << " | ";
            std::cout << "POLLERR";
            first = false;
        }
    }
    std::cout << std::endl;
}


bool setNonBlocking(int fd)
{
    int flags = fcntl(fd, F_GETFL, 0);
    if (flags == -1)
    {
        std::cerr << "fcntl F_GETFL failed" << std::endl;
        return false;
    }
    return fcntl(fd, F_SETFL, flags | O_NONBLOCK) != -1;
}


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

    // set socket to non-blocking mode
    if (!setNonBlocking(server_fd   ))
    {
        std::cerr << "Failed to set non-blocking" << std::endl;
        close(server_fd);
        return 1;
    }

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
    // initi poll array with server sockets
    std::vector<struct pollfd> poll_fds;
    struct pollfd server_pfd;
    server_pfd.fd = server_fd;
    server_pfd.events = POLLIN;
    server_pfd.revents = 0;
    poll_fds.push_back(server_pfd);

    std::cout << "🔧 Initial Setup:" << std::endl;
    std::cout << "   Server fd: " << server_fd << std::endl;
    printEvents("   Server events", server_pfd.events);
    printEvents("   Server revents", server_pfd.revents);
    std::cout << std::endl;

    // int iteration = 0;
    // while (true)
    // {
    //     iteration++;
    //     std::cout << "🔄 === Poll Iteration #" << iteration << " ===" << std::endl;

    //     // show current state before poll()
    //     std::cout << "📊 Before poll() - Monitoring " << poll_fds.size() << " file descriptors:" << std::endl;
    //     for (size_t i = 0; i < poll_fds.size(); ++i)
    //     {
    //         std::cout << "   [" << i << "] fd=" << poll_fds[i].fd;
    //         if (poll_fds[i].fd == server_fd)
    //         {
    //             std::cout << " (SERVER)";
    //         } else {
    //             std::cout << " (CLIENT)";
    //         }
    //         std::cout << std::endl;
    //         printEvents("       events ", poll_fds[i].events);
    //         printEvents("       revents", poll_fds[i].revents);
    //     }

    //     std::cout << "⏳ Calling poll() with timeout = -1 (wait forever)..." << std::endl;
    //     // call poll()
    //     int ready = poll(&poll_fds[0], poll_fds.size(), -1);
    //     std::cout << "⚡ poll() returned: " << ready << " file descriptors ready" << std::endl;
    //     // error case
    //     if (ready == -1) {
    //         std::cerr << "❌ poll() failed" << std::endl;
    //         break;
    //     }
    //     // Show what poll() detected
    //     std::cout << "🔍 After poll() - Events detected:" << std::endl;
    //     for (size_t i = 0; i < poll_fds.size(); ++i) {
    //         std::cout << "   [" << i << "] fd=" << poll_fds[i].fd;
    //         if (poll_fds[i].fd == server_fd) {
    //             std::cout << " (SERVER)";
    //         } else {
    //             std::cout << " (CLIENT)";
    //         }
    //         std::cout << std::endl;
    //         printEvents("       events ", poll_fds[i].events);
    //         printEvents("       revents", poll_fds[i].revents);
            
    //         if (poll_fds[i].revents != 0) {
    //             std::cout << "       ⭐ THIS FD HAS EVENTS!" << std::endl;
    //         }
    //     }
    // }

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