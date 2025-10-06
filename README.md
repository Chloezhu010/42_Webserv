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
    - `getaddrinfo` function: convert string repre of hostnames, host addr, service names, and port nbr into socket addr structure
        - `int getaddrinfo (const char *host, const char *service, const struct addrinfo *hints, struct addrinfo **result);`: return 0 if ok, nonzero error code on error
            - Given host & service, returns a result that points to a linked list of addrinfo structure
        - `void freeaddrinfo (struct addrinfo *result);`: return nothing, to avoid memory leaks
        - `const char *gai_strerror (int errcode);`: return error msg

            ![alt text](<img/Screenshot 2025-08-16 at 19.13.47.png>)
    - Wrapped helper function: `open_clientfd`: A client establishs a connection with a server by calling it
        - `int open_clientfd (char *hostname, char *port);`: return fd if ok, -1 on error
    - Wrapped helper function: `open_listenfd`: A server creates a listening fd that is ready to receive connection requests by calling it
        - `int open_listenfd (char *port);`: return fd if ok, -1 on error
- Web basics
    - HTTP (hypertext transfer protocol): a text-based app-level protocol used by web clients & servers to interact
        - A web client (eg. browser) opens an internet connection to a server and request some content
        - The server responds with the requested content, then closes the connection
        - The browser reads the cotnent and displays it on the screen
    - Web servers provide content to clients in 2 ways
        - Fetch a disk file and return its contents to the client: aka static content & serving static content
        - Run an executable file and return its output to the client: aka dynamic content & serving dynamic content
        - Each of these files has a unique URL (universal resource locator)
    - HTTP transactions
        - Use telnet to conduct transactions with any web server on the internet
        ```
        telnet www.google.com 80
        ....
        GET / HTTP/1.1
        Host: www.google.com
        ```
        - Telnet protocol is largely replaced by SSH as a remote login tool, but it's very handy for debugging servers that talks to clients with text lines over connections
    - HTTP requests
        - Common methods: GET, POST, OPTIONS, HEAD, PUT, DELETE, TRACE
    - HTTP responses
        - Common status code: 200, 301, 400, 403, 404, 501, 505
- Web server architecture types
    - Process-based
        - Traditional fork/ pre fork: eg. Apache http server
        - Modern process-based: eg. Unicorn - Github
    - Thread-based
        - Thread-per-connection/ Thread pool: Apache tomcat - Linkedin
    - Event-driven
        - Single-threaded event loop: Node.js - Paypal
        - Multi-process event-driven: Nginx
    - Hybrid & modern ones
        - Event-driven + thread pool: Netty - Twitter
        - Coroutine/ Fiber-based: Go servers - Docker, Erlang/Elixir - Whatsapp/ Discord
        - Async/ Await: Rust async - Dropbox/ Discord

- Nginx architecture
    - High level: Event-driven, async, non-blocking architecture
    - Core architecture
        - Master-worker process
            - One master process manages config, binds to ports, and spawns worker process
            - Multiple worker processes (usually 1/cpu core) handle all client requests
            - Workers are single-threaded but use async IO to handle thousands of connections simultaneously
        - Event loop with epoll/ kqueue
            - Each worker runs an event loop that monitors fd for IO events
            - Use epoll (linux) or kqueue (bsd/ macos) to avoid polling overhead
            - Can handle 10,000+ concurrent connections per worker with minimal memory usage
        - State machine approach
            - HTTP request processing breaks into discrete state (read request, parse headers, generate response etc.)
            - Each connection progresses through states as IO becomes available
            - No blocking operations - if data isn't ready, the connection is parked until it is
        - Memory pools
            - Custom memory mgmt with pre-allocated pools to reduce malloc/ free overhead
            - Mempools are reset btw requests rather than individually freed
    - Ref
        - https://blog.nginx.org/blog/inside-nginx-how-we-designed-for-performance-scale
        - https://nginxblog-8de1046ff5a84f2c-endpoint.azureedge.net/blobnginxbloga72cde487e/wp-content/uploads/2024/12/150427_NGINX_Architecture_IG_CMYK.pdf
        - https://aosabook.org/en/v2/nginx.html
- CGI (Common Gateway Interface)
    - A standard protocol that defines how web servers communicate with external programs to generate dynamic web content. A bridge between static web servers and dynamic apps.
    - Example request flow:
        ```
        // static content - server just reads a file and sends it
        Browser requests /hello.html → Server reads hello.html → Sends file content
        
        // dynamic content - server runs a program that generates content on-the-fly
        Browser requests /hello.php → Server runs PHP program → PHP generates HTML → Sends generated content
        ```
## Eval
### Concepts
- Basic of HTTP server
    - A program that listens for http requests and send back http response
    - Client -> send http request -> server -> generate http response -> client
- Funtion used for I/O multiplexing
    - select(), works on all posix system, macOS, linux, BSD
    - IO multiplexing: monitor multiple file descriptors (sockets) simultaneously to see which ones are ready for I/O operations -> non-blocking
- How select() is working: in run()
    - Phase 1: setup, clear fd_sets from previous iteration
    - Phase 2A: Monitor server sockets, detect new incoming connections
    - Phase 2B: Monitor client sockets, monitor for incoming data and write readiness
    - Phase 3: use select() for multiplexing
        - select() sleeps until one of these occurs
            - any fd in readFds has data to read
            - any fd in the writeFds is ready to write
            - 100ms timeout expires -> how long select() waits before giving up and returning
        - return
            - > 0: number of ready fds
            - = 0: timeout
            - < 0: error
    - Phase 4: handle ready connection
    - Phase 4A: read requests
    - Phase 4B: write responses
    - Phase 4C: connection cleanup
- select() is in the main loop and checks read end & write end simultaneously
    - ```int activity = select(maxFd + 1, &readFds, &writeFds, NULL, &timeout);```



### Port issues
- Launch multiple servers at the same time on the common port
    - When the 2nd server also tries to bind with port 8080, it will return "Address already in use"
### Siege test
- Siege: http load testing & benchmarking tool that simulates multiple concurrent users hitting the server
- What it tests
    - Concurrent connections: How many simultaneous users your server can handle
    - Throughput: Requests per second
    - Response time: How fast your server responds under load
    - Availability: Percentage of successful requests (uptime)
    - Stability: Whether your server crashes or hangs under stress
- Start test
    ```
    # Basic stress test (10 concurrent users, 30 seconds)
    siege -c 10 -t 30s http://localhost:8080/

    # More aggressive (50 users, 1 minute)
    siege -c 50 -t 1m http://localhost:8080/
    ```
    
## Reference sources
- RFC: https://www.rfc-editor.org/
- CSAPP: https://csapp.cs.cmu.edu/
    - Book: https://www.cs.sfu.ca/~ashriram/Courses/CS295/assets/books/CSAPP_2016.pdf


