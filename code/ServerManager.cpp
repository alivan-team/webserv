
#include "./hpp/ServerManager.hpp"

ServerManager::ServerManager(const std::vector<ServerConfig>& servers) : _servers(servers) {};

const std::vector<ServerConfig>& ServerManager::getServerManager() const {
    return _servers;
};

void ServerManager::acceptNewClient(int serverFd) {

    int newClientFd = accept(serverFd, NULL, NULL);

    if (newClientFd < 0)
        return ;

    setNonBlocking(newClientFd);

    pollfd client_poll;
    client_poll.fd = newClientFd;
    client_poll.events = POLLIN;
    client_poll.revents = 0;

    _pollfds.push_back(client_poll);
    _clients[newClientFd] = Client(newClientFd, serverFd);

    std::cout << "New client: fd " << newClientFd << "\n";
};

void ServerManager::readClientData(size_t index) {
    // here were we call the recieve client class

    // Client class

    // HTTPrequestParser?
        // if POST control length;
    
    // Response 


    int clientFd = _pollfds[index].fd;

    char buffer[4096];
    std::memset(buffer, 0, sizeof(buffer));

    int bytes = recv(clientFd, buffer, sizeof(buffer) - 1, 0);

    if (bytes <= 0)
    {
        std::cout << "Client disconnected fd: " << clientFd << std::endl;

        close(clientFd);
        _pollfds.erase(_pollfds.begin() + index);
        return;
    }

    Client& client = _clients[clientFd];
    client.appendToRequestBuffer(buffer, bytes);

    // if (!client.hasCompleteRequest())
        // return ;


    std::cout << "~~~~~~ REQUEST ~~~~~~ \n\t client.getRequestBuffer() \n\t -- from fd : " << clientFd << " -- \n";
    std::cout << client.getRequestBuffer() << std::endl;

    std::string body = "<h1>Hello from ServerManager</h1>\n";

    std::string response =
        "HTTP/1.1 200 OK\r\n"
        "Content-Type: text/plain\r\n"
        "Content-Length: " + std::to_string(body.size()) + "\r\n"
        "\r\n" +
        body;

    send(clientFd, response.c_str(), response.size(), 0);

    close(clientFd);
    _pollfds.erase(_pollfds.begin() + index);
};

bool ServerManager::isServerSocket(int fd) const
{
    for (size_t i = 0; i < _serverSockets.size(); ++i)
    {
        if (_serverSockets[i] == fd)
            return true;
    }
    return false;
}

void ServerManager::initialize() {

    if(_servers.empty())
        throw std::runtime_error("No servers configured");
    
    for (size_t i = 0; i < _servers.size(); i++) {
        createListeningSockets(_servers[i]);
    }
};

void ServerManager::run() {

    while (true) {
        
        int ready = poll(_pollfds.data(), _pollfds.size(), -1);
        if (ready < 0)
            throw std::runtime_error("poll() failed");

        for (size_t i = 0; i < _pollfds.size(); i++) {

            if (_pollfds[i].revents & POLLIN) {

                if (isServerSocket(_pollfds[i].fd)) {
                    acceptNewClient(_pollfds[i].fd);
                } else {
                    readClientData(i);
                }
            }
        }
    }
};

void ServerManager::setNonBlocking(int fd)
{
    int flags = fcntl(fd, F_GETFL, 0);

    if (flags < 0)
        throw std::runtime_error("fcntl(F_GETFL) failed");

    if (fcntl(fd, F_SETFL, flags | O_NONBLOCK) < 0)
        throw std::runtime_error("fcntl(F_SETFL) failed");
}

void ServerManager::createListeningSockets(const ServerConfig& servers) {

    int server_fd = socket(AF_INET, SOCK_STREAM, 0);

    if (server_fd < 0) 
        throw std::runtime_error("socket() failed");
    
    int opt = 1;

    if (setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt)) < 0) {
        close(server_fd);
        throw std::runtime_error("setsockopt() failed");
    }

    sockaddr_in addr;
    std::memset(&addr, 0, sizeof(addr));
    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = INADDR_ANY;

    int port = servers.getPort();

    addr.sin_port = htons(port);

    if (bind(server_fd, reinterpret_cast<sockaddr*>(&addr), sizeof(addr)) < 0) {
        close(server_fd);
        throw std::runtime_error("bind() failed");
    }

    if (listen(server_fd, 128) < 0) { 
        close(server_fd);
        throw std::runtime_error("listen() failed");
    }

    setNonBlocking(server_fd);

    _serverSockets.push_back(server_fd);

    pollfd server_poll;
    server_poll.fd = server_fd;
    server_poll.events = POLLIN;
    server_poll.revents = 0;

    _pollfds.push_back(server_poll);

    std::cout << "Listening on port: " << port << std::endl;

};

