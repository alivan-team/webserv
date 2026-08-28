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
	} catch (const std::exception& e) {
		std::cerr << "Error: " << e.what() << "\n";
		return 1;
	}

	return 0;

	
}