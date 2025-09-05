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
    User submits form → Web Server → CGI Script → Database/Processing → HTML Output → Web Server → User's Browser
    ```
- HTTP handling flow
    1. Socket data reception & buffering
        - Input: socket bytes from `handleClientRead()`
        - Buffer management
            - Append Strategy: Add new bytes to existing buffer
            - Size Limits: Check against maximum request size (prevent DoS)
            - Incomplete Detection: Look for request termination markers
            - Memory Management: Prevent buffer overflow attacks
        - Request completeness detection
            - For GET/DELETE requests:
                - Look for \r\n\r\n (double CRLF) - headers end marker
                - Once found, request is complete
            - For POST requests:
                - First find \r\n\r\n (headers complete)
                - Extract Content-Length from headers
                - Continue reading until header_length + content_length bytes received
                - Handle case where body arrives in multiple chunks
        - Edge Cases:
            - Partial header reception (wait for more data)
            - Missing Content-Length in POST (return 400 Bad Request)
            - Content-Length mismatch (return 400 Bad Request)
    2. HTTP request parsing
        - Input: complete HTTP request string
        - Request-line parsing
            - Parse pattern: `METHOD URI HTTP/VERSION\r\n`
            - Validation logic
                - Split by space
                - Method validation
                - URL validation
                - HTTP version
        - Header parsing logic
            - Parse pattern: `Header-Name: Header-Value\r\n`
            - Parsing algo
                - Split by line: \r\n as delimiter
                - For each header line
                    - Split into name & value based on `:`
                    - Trim whitespace
                    - Store in case-insensitive container
            - Header validation
                - For HTTP/1.1, Host header must be present
                - For POST, content-legnth required, content-type should be present
            - Special header processing
                - Connection: keep-alive vs close
                - Transfer-encoding
                - Content-length
        - Body parsing (POST/PUT)
            - Content-length strategy
            - Body validation
                - Size limit: check vs max body size config
                - Too large -> 413 payload too large
                - Negative length -> 400 bad request
    3. Request validation & security
        - Input: parsed HTTP request object
    4. Method routing & handler selection
        - Input: validated http request
        - Routing matching logic
            - Config-based routing
        - Handler selection
            - Decision tree
                ```
                Method = GET?
                ├─ Yes: Is path a file? → Static File Handler
                │       Is path a directory? → Directory Listing Handler
                │       
                ├─ No: Method = POST?
                │      ├─ Yes: Is CGI script? → CGI Handler  
                │      │       Is upload endpoint? → Upload Handler
                │      │       Default → Form Handler
                │      │
                │      └─ No: Method = DELETE?
                │             ├─ Yes: → Delete Handler
                │             └─ No: → 405 Method Not Allowed
                ```
    5. Handler execution
        - Static file handler (GET)
        - POST handler
        - DELETE handler
    6. Response building
        - Input: handler result + status code + content
        - Status line construction
            - Format: `HTTP/1.1 STATUS_CODE REASON_PHRASE\r\n`
            - Status code mapping
                ```
                200 → "OK"
                201 → "Created"
                204 → "No Content"
                400 → "Bad Request"
                403 → "Forbidden"
                404 → "Not Found"
                405 → "Method Not Allowed"
                500 → "Internal Server Error"
                ```
        - Header generation
            - Required headers: Date, Server, Content-Length
        - Response assembly
- Validation strategy
    - Pre-validation checks (safety check)
        - Input: raw request buffer
        - Check
            - Buffer size within limits?
            - Contain null byte or invalid char?
            - Request complete?
    - Request line validation
        - Method validation
        - URL validation
        - HTTP version validation
    - Header validation
        - Format check
    - Body validation
        - Method = GET/ DELETE: should have no body
        - Method = POST
            - Chunked body
            - Content-length body
            - No body
- HTTP request parsing
- HTTP response building
- HTTP methods
- HTTP status code
    
## Reference sources
- RFC: https://www.rfc-editor.org/
- CSAPP: https://csapp.cs.cmu.edu/
    - Book: https://www.cs.sfu.ca/~ashriram/Courses/CS295/assets/books/CSAPP_2016.pdf




## Corner cases for header validation
1. Required Headers

  - [ok] Missing Host header: HTTP/1.1 mandates Host header
  - [ok] Empty Host value: Host:  (empty after colon)
  - [ok] Multiple Host headers: Should be rejected

  2. Header Format Issues

  - [ok] Missing colon: Content-Length 100 (no :)
  - [ok] Empty header name: : value
  - [ok] Header name with spaces: Content Length: 100
  - [ok] Invalid characters in name: Content-Length\x00: 100

  3. Size Limits

  - [ok] Too many headers: > MAX_HEADER_COUNT (100)
  - [ok] Header line too long: > MAX_HEADER_SIZE (8KB)
  - [ok] Total headers too large: Sum of all headers > limit

  4. Content-Length Validation

  - [ok] Multiple Content-Length: Content-Length: 100\r\nContent-Length: 200
  - [ok] Invalid values: Content-Length: abc, Content-Length: -5
  - [ok] Conflicting with Transfer-Encoding: Both present simultaneously

  5. Transfer-Encoding Issues

  - Invalid encoding: Transfer-Encoding: gzip (only "chunked" supported)
  - Multiple encodings: Transfer-Encoding: chunked, gzip
  - Case variations: transfer-encoding: CHUNKED

  6. Connection Header

  - Invalid values: Connection: maybe (should be "keep-alive" or "close")
  - Case sensitivity: Connection: Keep-Alive vs keep-alive

  7. Method-Specific Header Requirements

  - [ok] GET/DELETE with Content-Length: Should be 0 or absent
  - [ok] POST without Content-Length/Transfer-Encoding: Missing body info

  8. Security Issues

  - Header injection: Values containing \r\n
  - Null bytes: Headers with \x00 characters
  - Very long values: Potential DoS attacks

  Most Critical to Validate First:

  1. Host header presence (HTTP/1.1 requirement)
  2. Content-Length/Transfer-Encoding conflicts
  3. Size limits (DoS prevention)
  4. Method-body consistency
