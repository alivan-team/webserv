// ConfigParser.hpp
#ifndef CONFIG_PARSER_HPP
#define CONFIG_PARSER_HPP

#include "ServerConfig.hpp"
#include <string>
#include <iostream>
#include <fstream>


class ConfigParser {

	private:
		typedef void (ServerConfig::*Setter)(const std::string&);

    	std::map<std::string, Setter> setters;
		
	    std::vector<ServerConfig> servers;
	
	public:
		ConfigParser();
		void parse(const std::string& filename);
		// const std::vector<ServerConfig>& getServers() const;

};

#endif
