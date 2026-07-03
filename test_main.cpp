#include <iostream>
#include <cstring>
#include <vector>
#include <unistd.h>
#include <poll.h>
#include <fcntl.h>
#include <sys/socket.h>
#include <netinet/in.h>




// Do NOT try to know the full size before recv().
// Use a fixed small buffer for each recv(),
// then append the received bytes into a dynamic per-client buffer.
// char tmp[4096];
// int bytes = recv(fd, tmp, sizeof(tmp), 0);
// if (bytes > 0)
// {
//     client.request_buffer.append(tmp, bytes);
// }

// How do we know when headers are complete?
// \r\n\r\n

// So after every recv(), you check:
// size_t header_end = client.request_buffer.find("\r\n\r\n");
// if (header_end != std::string::npos)
// {
//     // full HTTP headers received
// }

// Pseude-code for dealing with the buffer noncense.

// int bytes = recv(fd, tmp, sizeof(tmp), 0);

// if (bytes > 0)
// {
//     client.request_buffer.append(tmp, bytes);

//     size_t header_end = client.request_buffer.find("\r\n\r\n");

//     if (header_end == std::string::npos)
//         return; // wait for more data

//     // headers are complete
//     size_t body_start = header_end + 4;

//     int content_length = parseContentLength(client.request_buffer);

//     if (client.request_buffer.size() < body_start + content_length)
//         return; // body not complete yet

//     // full request received
//     processRequest(client.request_buffer);
// }

// accept 
// upon error return -1, set errno
//  check errno for possible explanation: 
//      EAGAIN
//      EWOULDBLOCK
//      EMFILE
//      ENFILE


static void setNonBlocking(int fd)
{
	int flags = fcntl(fd, F_GETFL, 0);
    // if (flags == -1)
    //     return;

    fcntl(fd, F_SETFL, O_NONBLOCK);
}

int main()
{

    // read config.file

    int server_fd = socket(AF_INET, SOCK_STREAM, 0);
	// if (server_fd == -1)
	// {
	// 	std::cerr << "socket() failed\n";
	// 	return 1;
	// }

    int opt = 1;
    setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));

    sockaddr_in addr;
    std::memset(&addr, 0, sizeof(addr));
    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = INADDR_ANY;
    addr.sin_port = htons(8080);

    if ((bind(server_fd, (sockaddr *)&addr, sizeof(addr))) == -1 ) {
        std::cout << "Error with binding" << "\n";
        exit(1);

    } 
    listen(server_fd, 128);

    setNonBlocking(server_fd);

    std::vector<pollfd> fds;

    pollfd server_poll;
    server_poll.fd = server_fd;
    server_poll.events = POLLIN;
    server_poll.revents = 0;

    fds.push_back(server_poll);

    std::cout << "Listening on http://localhost:8080\n";

    while (true)
    {
        poll(&fds[0], fds.size(), -1);

        for (size_t i = 0; i < fds.size(); ++i)
        {
            if (fds[i].revents & POLLIN)
            {
                if (fds[i].fd == server_fd)
                {
                    int client_fd = accept(server_fd, NULL, NULL);
                    // class for clinet 
                        // id = fd
                        // map
                    setNonBlocking(client_fd);

                    pollfd client_poll;
                    client_poll.fd = client_fd;
                    client_poll.events = POLLIN;
                    client_poll.revents = 0;

                    fds.push_back(client_poll);

                    std::cout << "New client: fd " << client_fd << "\n";
                }
                else
                {
                    char buffer[4096];
                    std::memset(buffer, 0, sizeof(buffer));

                    int bytes = recv(fds[i].fd, buffer, sizeof(buffer) - 1, 0);

                    // 1. recv() into temporary buffer
                    // 2. append to client.request_buffer
                    // 3. check if headers contain \r\n\r\n
                    // 4. if headers not complete: wait for more POLLIN
                    // 5. if headers complete:
                    //       parse request line + headers
                    // 6. if Content-Length exists:
                    //       wait until body size == Content-Length
                    // 7. if no body required:
                    //       process request

                    if (bytes <= 0)
                    {
                        std::cout << "Client disconnected\n";
                        close(fds[i].fd);
                        fds.erase(fds.begin() + i);
                        --i;
                    }
                    else
                    {
                        std::cout << "Request from fd " << fds[i].fd << ":\n";
                        std::cout << "~~~~~~~~~~~~~~~~BUFFER~~~~~~~~~~~~~~~~" << "\n";
                        std::cout << buffer << "\n";

                        std::string body = "Hello from poll server\n";

                        std::string response =
                            "HTTP/1.1 200 OK\r\n"
                            "Content-Type: text/plain\r\n"
                            "Content-Length: " + std::to_string(body.size()) + "\r\n"
                            "\r\n" +
                            body;

                        send(fds[i].fd, response.c_str(), response.size(), 0);
                    }
                }
            }
        }
    }
}
///////////////////////////////////////////////////////

void ServerManager::setNonBlocking(int fd)
{
    int flags = fcntl(fd, F_GETFL, 0);

    if (flags < 0)
        throw std::runtime_error("fcntl(F_GETFL) failed");

    if (fcntl(fd, F_SETFL, flags | O_NONBLOCK) < 0)
        throw std::runtime_error("fcntl(F_SETFL) failed");
}

void ServerManager::createListeningSocket(const ServerConfig& server)
{
    int server_fd = socket(AF_INET, SOCK_STREAM, 0);
    // internally the kernel creates a socket structure 
    // Your process
    // -----------
    // server_fd = 3

    // Kernel
    // ------
    // fd table:
    // 3 -> socket object

    // socket object:
    //     address family: IPv4
    //     type: TCP stream
    //     local IP: not set yet
    //     local port: not set yet
    //     state: created, but not listening
    //
    // AF_INET - use IPv4
    // SOCK_STREAM - use stream-based connection -> in practice with AF_INET this means TCP
    //      TCP means byte stream, not separate messages
    // 0 - Choose the default protocol for this family and type 
    //      in this case that is TCP

    if (server_fd < 0)
        throw std::runtime_error("socket() failed");

    int opt = 1;

    if (setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt)) < 0)
    {
        close(server_fd);
        throw std::runtime_error("setsockopt() failed");
    }
    // setsockopt() -> "Kernel, change one of the settings of this socket."
    // setsockopt() has this prototype:

    // int setsockopt(
    //     int sockfd,
            //  which socket;

    //     int level,
            // Which protocol layer owns this option.
            // SOL_SOCKET -  meaning: This option belongs to the generic socket.
            // some layers are owned by different options:
                // Socket layer:
                // TCP layer:
                // IP layer
    
    //     int option_name,
            //  This is the option I'm chaning : SO_REUSEADDR / Which option to chage;
            //  this is not a value: just saying to the kernel find the name of this option: the option_value change is it.

    //     const void *option_value,
            // pointer to a new value for the option: 
            // by default this option is 0 (disabled) -> so 1 (enable);
    
    //     socklen_t option_len
            //  size of the option_value;
    // );

    sockaddr_in addr;
    // preparing an address structure to tell the kernel more details about the struct socket.
    std::memset(&addr, 0, sizeof(addr));

    addr.sin_family = AF_INET;
    // means this address is AF_INET = IPv4
    addr.sin_addr.s_addr = INADDR_ANY;
    // menas listen to all available local IP addresses;
    // it will bind to all the available [network rooms... ( my analogy )] local addresses.

    // adapt this depending on your getter
    int port = server.getPort()[0];

    addr.sin_port = htons(port);
    // htons converts the host byte order to network byte order. bit small-> network must be Big Endian.


    if (bind(server_fd, reinterpret_cast<sockaddr*>(&addr), sizeof(addr)) < 0) {
        // bind takes the socket indentifier by server_fd and assigns it to this local address
        // and associates it with the socket object that we have created earler;
        close(server_fd);
        throw std::runtime_error("bind() failed");
    }

    if (listen(server_fd, 128) < 0) {
        // the second parameter of listen is how big the queue cna be... 129 queue is full ...
        close(server_fd);
        throw std::runtime_error("listen() failed");
    }

    setNonBlocking(server_fd);

    _serverSockets.push_back(server_fd);

    pollfd pfd;
    // 
    pfd.fd = server_fd;
    pfd.events = POLLIN;
    // Wake me up when this fd becomes readable.
    pfd.revents = 0;
    // 

    _pollfds.push_back(pfd);

    std::cout << "Listening on port " << port << std::endl;
}