#include "./code/hpp/ServerConfig.hpp"
#include "./code/hpp/ConfigParser.hpp"
#include "./code/hpp/ServerManager.hpp"
#include "./code/hpp/printDebug.hpp"

#include <iostream>
#include <exception>


int main(int argc, char **argv) {

    std::string configPath;
    if (argc == 1)
        configPath = "./config/default.conf";
    else if (argc == 2)
        configPath = argv[1];
    else {
        std::cerr << "Usage: ./webserv [config_file]\n";
        return 1;
    }

    ConfigParser config;

    try {
        config.parse(configPath);
        ServerManager socketManager(config.getServers());
        socketManager.initialize();
        socketManager.run();


        const std::vector<ServerConfig>& servers = socketManager.getServerManager();

        for (size_t i = 0; i < servers.size(); ++i)
        {
            std::cout << "-|-|-|-|-|-|-|-|-|-|-|-|-|-|-|-|-\n";

            std::cout << "Server " << i << "\n";

            printDebug("Ports: ", servers[i].getPort());

            printDebug("Server names: ", servers[i].getServerName());

            printDebug("Roots: ", servers[i].getRoot());

            printDebug("Indexes: ", servers[i].getIndex());

            printDebug("Client max body size: ", servers[i].getClientMaxBodySize());

            printDebug("Error page: ", servers[i].getErrorPage());

            std::cout << "Locations \n";
            for (size_t j = 0; j < servers[i]._locations.size(); ++j) {

                printDebug("(", j);
                std::cout << ")\n";
                printDebug("", servers[i]._locations[j]);
            }

            std::cout << "-----------------\n";
        }
    } catch (const std::exception& e) {
        std::cerr << "Error: " << e.what() << "\n";
        return 1;
    }

    return 0;
}