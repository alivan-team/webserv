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


// ok and now here: 



// void ConfigParser::parse(const std::string& filename)

// {

//     std::ifstream file(filename.c_str());



//     if (!file) {

//         throw std::runtime_error("Cannot open config file");

// 	}

// 	std::vector<std::string> configTokens = tokenize(file);

//     // MAP -> asdad array [1,1] =  ([]"listen", *setPort], ["server_name", *setServerName])

//     for (int i = 0; i < configTokens.size(); i++) {



//         if (configTokens[i] == "server" && i + 1 < configTokens.size() && configTokens[i + 1] == "{") {

            

//             ServerConfig server;

//             i += 2;

           

//             while (i  < configTokens.size() && configTokens[i] != "}" ) {

                

//                 try {



//                 } catch(std::string exeption ) {

//                     throw ;

//                 }

//             }



//             servers.push_back(server);

//         }

    

//     }



// in try I have to put call the setters