#ifndef SERVERMANAGER_HPP
#define SERVERMANAGER_HPP

#include "ConfigParser.hpp"

class ServerManager {

    private:
        std::vector<ServerConfig> _servers;
        // std::vector<int> _serverSockets;
        // std::vector<pollfd> _pollfds;
        // void createListeningSockets();
        // void acceptClient(int serverFd);
        // void receiveRequest(int clientFd);
        // void sendResponse(int clientFd);
        // void removeClient(int clientFd);

    public: 
        ServerManager(const std::vector<ServerConfig>& servers);
        const std::vector<ServerConfig>& getServerManager() const;
        // void initialize();
        // void run();
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