#ifndef SERVERMANAGER_HPP
#define SERVERMANAGER_HPP

#include "ConfigParser.hpp"
#include "ClientData.hpp"
#include <iostream>
#include <cstring>
#include <vector>
#include <unistd.h>
#include <poll.h>
#include <fcntl.h>
#include <sys/socket.h>
#include <netinet/in.h>

class ServerManager {

    private:
        // std::vector<ServerConfig> _servers;
        std::map<int, ServerConfig> _serversMap;
        std::vector<int> _serverSockets;
        std::vector<pollfd> _pollfds;
        std::map<int, Client> _clients;
        void setNonBlocking(int fd);
        int createListeningSockets(const ServerConfig& servers);
        bool isServerSocket(int fd) const;
        void acceptNewClient(int serverFd);
        void readClientData(size_t index);

        // void receiveRequest(int clientFd);
        // void sendResponse(int clientFd);
        // void removeClient(int clientFd);

    public: 
        // ServerManager(const std::vector<ServerConfig>& servers);
        // ServerManager();
        const std::map<int, ServerConfig>& getServerManager() const;
        const ServerConfig& getClientServerManager(int serverIndex) const;
        void initialize(const std::vector<ServerConfig>& servers);
        void run();
};

#endif

// class ServerManager
// {
// private:

//     std::vector<ServerConfig> _servers;
//     std::vector<int> _serverSockets;
//     std::vector<pollfd> _pollfds;

// public:

//     ServerManager(const std::vector<ServerConfig>&);
//     void initialize();
//     void run();
// };