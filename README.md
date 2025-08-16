# 42_Webserv
## Project Overview
Create a complete HTTP server from scratch in C++98 that can:
- Handle real web browser requests
- Serve static websites
- Support file uploads/downloads
- Execute CGI scripts (PHP, Python, etc.)
- Manage multiple simultaneous connections
- Parse configuration files
## Core skill sets
- System programming
    - Non-blocking I/O with poll()/select()/epoll()
    - Socket programming and network protocols
    - Process management and inter-process communication
    - File descriptor management
    - Signal handling
- Web technologies
    - Deep understanding of HTTP/1.1 protocol
    - Request/response parsing and generation
    - Status codes, headers, and content handling
    - CGI (Common Gateway Interface) implementation
    - Web server architecture patterns
## Key concepts
### HTTP protocol fundamentals
- RPC protocol
- MDN web doc: https://developer.mozilla.org/en-US/docs/Web/HTTP/Reference/Resources_and_specifications
- Practicals
    - Use telnet to manually send HTTP requests: telnet google.com 80
    - Examine requests/responses in browser dev tools
    - Use curl -v to see detailed HTTP exchanges
### Web server architecture
- Study existing servers
    - Nginx doc
    - Apache http server doc
- Architecture patterns
    - Event driven vs thread-per-connection models
    - Reactor pattern
    - State machines for protocol handling
### CGI (Common gateway interface)
- RFC 3875
- How PHP-CGI works
### Network programming in C++
- Socket creation & binding
- listen(), accept(), send(), recv()
- Non-blocking I/O with poll(), select(), epoll()
- Address families and sockaddr structures
## Notes
- Client-server transaction 4 steps
    - Client send request to server
    - Server processes request
    - Server send responses to client
    - Client processes responses

        ![alt text](<img/Screenshot 2025-08-16 at 19.01.00.png>)
- Internet Protocol: A layer of protocol software running on each host & router that smoothes out the differences btw different networks (LAN, WANs taht wuse radically different & incompatible technologies)
    - **Naming scheme**: The internet protocol define a uniform format for host addresses. Each host is assigned at least one of these internet addresses taht uniquely identifies it
    - **Delivery mechanism**: Define a uniform way to bundle up data bits into packets. A packet coonsists of a header, and a payload.
- Internet connection
    - Internet clients and servers communicate by sending & receiving streams of bytes over connections.
    - A socket is an end point of a connections. Each socket has a corresponding socket address, consisting of an internet address and a port: `address:port`
        - The port in the client's socket address is assigned auto by the kernel when the client makes a connection request, and is known as an ephermeral port
        - The port in the server's socket address is typically well-known port eg. 80, 25
- Sockets interface

    ![alt text](<img/Screenshot 2025-08-16 at 17.01.17.png>)
- Networking programming
    - `socket` function: client and server use the socket function to create a socket descriptor
        - `int socket (int domain, int type, int protocol);`: return non-negative descriptor if ok, -1 on error
        - The returned descriptor is only partially opened and cannot yet be used for reading/ writing, until the connection succeed
    - `connect` function: A client establishes a connection with a server by calling the connect function
        - `int connect (int clientfd, const struct socketaddr *addr, socklen_t addrlen);`: return 0 if ok, -1 on error
        - The resulting connection is featured by the socket pair: `x:y, addr.sin_addr:addr.sin_port`
        - The best practice is to use `getaddrinfo` to supply the args to `connect`
    - `bind` function: used by server to establish connections with clients
        - `int bind (int sockfd, const struct sockaddr *addr, socklen_t addrlen);`: return 0 if ok, -1 on error
        - Ask the kernel to associate the server's socket address in `addr` with the socket descript `sockfd`
    - `listen` function: By default, the kernel assumes that a descriptor created by the socket function corresponds to an active socket that will live on client end. A server calls the `listen` function to tell the kernel that the descriptor will be used by a server.
        - `int listen (int sockfd, int backlog);`: return 0 if ok, -1 on error
        - backlog arg is a hint about the # of outstanding connection requests that the kernel should queue up before it starts to refuse requests.
    - `accept` function: Server waits for connection requests from clients by calling the accept function
        - `int accept (int listenfd, struct sockaddr *addr, int *addrlen);`: return non-negative connected descriptor if ok, -1 on error
        - Process
            - Server calls `accept` and waits for a connection request to arrive on the `listenfd`
            - Client calls `connect`, which sends a connection request to `listenfd`
            - `accept` function opens a new connected fd `connfd`, establishes the connection between `clientfd` and `connfd`, then returns `connfd` to the application
            ![alt text](<img/Screenshot 2025-08-16 at 19.00.07.png>)
        - Diff between a listen fd and connected fd
            - listenfd: an end point for client connection requrests; typically created once and exists for the lifetime of the server
            - connfd: an end point of the connection that is established btw the client and server; created each time the server accepts a connection request, exists only as long as it takes the server to service a client

## Reference sources
- CSAPP: https://csapp.cs.cmu.edu/