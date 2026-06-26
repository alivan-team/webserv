#include "./code/hpp/ServerConfig.hpp"
#include "./code/hpp/ConfigParser.hpp"

#include <iostream>
#include <exception>

int main(int argc, char **argv)
{

    std::string configPath;
    if (argc == 1)
        configPath = "./config/default.conf";
    else if (argc == 2)
        configPath = argv[1];
    else
    {
        std::cerr << "Usage: ./webserv [config_file]\n";
        return 1;
    }

    ConfigParser config;

    try {
        config.parse(configPath);
    } catch (const std::exception& e) {
        std::cerr << "Config error: " << e.what() << "\n";
        return 1;
    }

    const std::vector<ServerConfig>& servers = config.getServers();

    for (size_t i = 0; i < servers.size(); ++i)
    {
        std::cout << "Server " << i << "\n";
        std::cout << "Port: " << servers[i].getPort() << "\n";
        std::cout << "Server name: " << servers[i].getServerName() << "\n";
        std::cout << "Root: " << servers[i].getRoot() << "\n";
        std::cout << "Index: " << servers[i].getIndex() << "\n";
        std::cout << "-------------------\n";
    }

    // std::cout << "Port: " << config.port << "\n";
    // std::cout << "Server name: " << config.server_name << "\n";
    // std::cout << "Root: " << config.root << "\n";
    // std::cout << "Index: " << config.index << "\n";
    // std::cout << "Locations: " << config.locations.size() << "\n";

    return 0;
}