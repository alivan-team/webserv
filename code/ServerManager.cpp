
#include "./hpp/ServerManager.hpp"

ServerManager::ServerManager(const std::vector<ServerConfig>& servers) : _servers(servers) {};

const std::vector<ServerConfig>& ServerManager::getServerManager() const {
    return _servers;
};