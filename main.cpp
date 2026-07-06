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
        ServerManager socketManager;
        socketManager.initialize(config.getServers());
        socketManager.run();


        const std::map<int, ServerConfig>& servers = socketManager.getServerManager();

        for (auto serv : servers)
        {
            std::cout << "-|-|-|-|-|-|-|-|-|-|-|-|-|-|-|-|-\n";

            std::cout << "Server " << serv.second.getServerConfFD() << "\n";

            printDebug("Ports: ", serv.second.getPort());

            printDebug("Server names: ", serv.second.getServerName());

            printDebug("Roots: ", serv.second.getRoot());

            printDebug("Indexes: ", serv.second.getIndex());

            printDebug("Client max body size: ", serv.second.getClientMaxBodySize());

            printDebug("Error page: ", serv.second.getErrorPage());

            std::cout << "Locations \n";
            for (size_t j = 0; j < serv.second._locations.size(); ++j) {

                printDebug("(", j);
                std::cout << ")\n";
                printDebug("", serv.second._locations[j]);
            }

            std::cout << "-----------------\n";
        }
    } catch (const std::exception& e) {
        std::cerr << "Error: " << e.what() << "\n";
        return 1;
    }

    return 0;
}