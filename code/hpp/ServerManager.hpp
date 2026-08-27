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
#include <cerrno>
#include <netdb.h>

class ServerManager {

    private:
        std::map<int, std::vector<ServerConfig>> _serversMap;
        std::vector<int> _serverSockets;
        std::vector<pollfd> _pollfds;
        std::map<int, Client> _clients;
        void setNonBlocking(int fd);
        int findServerFd(const std::string& host, int port) const;
        int createListeningSockets(const ServerConfig& servers);
        bool isServerSocket(int fd) const;
        void acceptNewClient(int serverFd);
        bool readClientData(size_t index);
        bool writeClientData(size_t index);

        bool shouldKeepAlive(const HTTPRequest& request) const;
        void removeClient(size_t index);
        // bool sendWholeResponse(int clinetFd, const std::string& respone) const;
        bool processRequestBuffer(size_t index);
        RequestState getRequestState(Client& client, const ServerConfig*& serverConfig);

    public: 
        void queueResponse(size_t index, Client& client, HTTPResponse& response);
        const std::map<int, std::vector<ServerConfig>>& getServerManager() const;
        const ServerConfig& getClientServerManager(int serverIndex, const std::string& host) const;
        void initialize(const std::vector<ServerConfig>& servers);
        void run();
};

#endif
