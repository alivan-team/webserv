#ifndef SERVER_CONFIG_HPP
#define SERVER_CONFIG_HPP

#include <iostream>
#include <vector>
#include <exception>
#include <string>
#include <map>
#include <stdexcept>
#include <cctype>
#include "LocationConfig.hpp"

class ServerConfig {

    private:
        std::vector<int> _port;
        // int _client_max_body_size;
        std::vector<std::string> _server_name;
        std::vector<std::string>  _root;
        std::vector<std::string>  _index;
        std::vector<LocationConfig> _locations;
        std::map<int, std::string> _error_pages;

    public:
        ServerConfig();
        ~ServerConfig();

        void setPort(const std::vector<std::string>& port);
        void setServerName(const std::vector<std::string>& server_name);
        void setRoot(const std::vector<std::string>& root);
        void setIndex(const std::vector<std::string>& index_name);

        const std::vector<int>& getPort() const;
        const std::vector<std::string>& getServerName() const;
        const std::vector<std::string>& getRoot() const;
        const std::vector<std::string>& getIndex() const;

        bool parsePort(const std::string& port);



};

#endif
