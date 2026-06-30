#include "./code/hpp/ServerConfig.hpp"
#include "./code/hpp/ConfigParser.hpp"

#include <iostream>
#include <exception>

void printVector(const std::vector<int>& vec)
{
    for (int value : vec)
        std::cout << value << " ";

    std::cout << "\n";
}

void printVector(const std::vector<unsigned int>& vec)
{
    for (int value : vec)
        std::cout << value << " ";

    std::cout << "\n";
}

void printVector(const std::vector<std::string>& vec)
{
    for (const auto& value : vec)
        std::cout << value << " ";

    std::cout << "\n";
}

void printVector(const std::map<int, std::string>& vec)
{
    for (const auto& value : vec) {
        std::cout << "\t code: " << value.first << " | path: " << value.second << " ";
        std::cout << "\n";
    }

}

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

    // here 

    const std::vector<ServerConfig>& servers = config.getServers();

    for (size_t i = 0; i < servers.size(); ++i)
    {
        std::cout << "Server " << i << "\n";

        std::cout << "Ports: ";
        printVector(servers[i].getPort());

        std::cout << "Server names: ";
        printVector(servers[i].getServerName());

        std::cout << "Roots: ";
        printVector(servers[i].getRoot());

        std::cout << "Indexes: ";
        printVector(servers[i].getIndex());

        std::cout << "Client max body size: ";
        printVector(servers[i].getClientMaxBodySize());

        std::cout << "Error page: ";
        printVector(servers[i].getErrorPage());

        std::cout << "Location: ";
        // printVector(servers[i].getLocation());

        std::cout << "-----------------\n";
    }

    // std::cout << "Port: " << config.port << "\n";
    // std::cout << "Server name: " << config.server_name << "\n";
    // std::cout << "Root: " << config.root << "\n";
    // std::cout << "Index: " << config.index << "\n";
    // std::cout << "Locations: " << config.locations.size() << "\n";

    return 0;
}