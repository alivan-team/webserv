// ConfigParser.hpp
#ifndef CONFIG_PARSER_HPP
#define CONFIG_PARSER_HPP

#include "ServerConfig.hpp"
#include <string>
#include <iostream>
#include <fstream>


class ConfigParser {

	private:
		using Setter = void (ServerConfig::*)(const std::vector<std::string>&);
		using LocationSetter = void (LocationConfig::*)(const std::vector<std::string>&);

    	std::map<std::string, Setter> setters;
    	std::map<std::string, LocationSetter> locationSetters;

	    std::vector<ServerConfig> servers;
	
	public:
		ConfigParser();
		void parse(const std::string& filename);
		const std::vector<ServerConfig>& getServers() const;

};

#endif
