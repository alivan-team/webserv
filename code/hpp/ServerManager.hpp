#ifndef SERVERMANAGER_HPP
#define SERVERMANAGER_HPP

#include "ConfigParser.hpp"
#include "ClientData.hpp"
#include "HelperFunctions.hpp"
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
        std::map<int, ServerConfig> _serversMap;
        std::vector<int> _serverSockets;
        std::vector<pollfd> _pollfds;
        std::map<int, Client> _clients;
        void setNonBlocking(int fd);
        int createListeningSockets(const ServerConfig& servers);
        bool isServerSocket(int fd) const;
        void acceptNewClient(int serverFd);
        bool readClientData(size_t index);

        bool shouldKeepAlive(const HTTPRequest& request) const;
        void removeClient(size_t index);
        bool sendWholeResponse(int clinetFd, const std::string& respone) const;

    public: 
        const std::map<int, ServerConfig>& getServerManager() const;
        const ServerConfig& getClientServerManager(int serverIndex) const;
        void initialize(const std::vector<ServerConfig>& servers);
        void run();
};

#endif
