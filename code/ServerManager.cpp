
#include "./hpp/ServerManager.hpp"

ServerManager::ServerManager(const std::vector<ServerConfig>& servers) : _servers(servers) {};

const std::vector<ServerConfig>& ServerManager::getServerManager() const {
    return _servers;
};

void ServerManager::initialize() {

    if(_servers.empty())
        throw std::runtime_error("No servers configured");
    
    for (size_t i = 0; i < _servers.size(); i++) {
        createListeningSockets(_servers[i]);
    }
};

void ServerManager::run() {
    
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

    pollfd pfd;
    pfd.fd = server_fd;
    pfd.events = POLLIN;
    pfd.revents = 0;

    _pollfds.push_back(pfd);

    std::cout << "Listening on port: " << port << std::endl;

};

