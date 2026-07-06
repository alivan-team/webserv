#ifndef SERVER_CONFIG_HPP
#define SERVER_CONFIG_HPP

#include <iostream>
#include <vector>
#include <exception>
#include <string>
#include <map>
#include <stdexcept>
#include <cctype>
#include <climits>
#include "LocationConfig.hpp"
#include <limits.h> 
#include <climits>

class ServerConfig {

    private:
        std::vector<int> _port;
        std::vector<std::string> _server_name;
        std::vector<std::string>  _root;
        std::vector<std::string>  _index;
        std::vector<unsigned int> _client_max_body_size;
        std::map<int, std::string> _error_pages;
        
        public:
        std::vector<LocationConfig> _locations;
        ServerConfig();
        ~ServerConfig();
        
        void setPort(const std::vector<std::string>& port);
        void setServerName(const std::vector<std::string>& server_name);
        void setRoot(const std::vector<std::string>& root);
        void setIndex(const std::vector<std::string>& index_name);
        void setClientMaxBodySize(const std::vector<std::string>& client_max_body_size);
        void setErrorPage(const std::vector<std::string>& error_page);
        
        const int& getPort() const;
        const std::vector<std::string>& getServerName() const;
        const std::vector<std::string>& getRoot() const;
        const std::vector<std::string>& getIndex() const;
        const std::vector<unsigned int>& getClientMaxBodySize() const;
        const std::map<int, std::string>& getErrorPage() const;

        // bool parsePort(const std::string& port);

		void addLocation(const LocationConfig &locations);
		const std::vector<LocationConfig>& getLocation() const;

};

#endif
