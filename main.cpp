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

    // std::cout << "Port: " << config.port << "\n";
    // std::cout << "Server name: " << config.server_name << "\n";
    // std::cout << "Root: " << config.root << "\n";
    // std::cout << "Index: " << config.index << "\n";
    // std::cout << "Locations: " << config.locations.size() << "\n";

    return 0;
}