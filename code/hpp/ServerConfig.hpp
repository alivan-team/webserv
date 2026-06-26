#ifndef SERVER_CONFIG_HPP
#define SERVER_CONFIG_HPP

#include <iostream>
#include <vector>
#include <exception>
#include <string>
#include <map>

class ErrorPages {

};

class LocationConfig {

    private:
        std::string path;                  // "/upload" -> CGI
        std::vector<std::string> methods;  // GET POST DELETE
        std::string upload_store;

    public:
        // LocationConfig();
        // ~LocationConfig();
        // evaluations for Location cofig
        check_path();
        setPath();
        getPath();

        
};

class ServerConfig {

    private:
        int _port;
        // int _client_max_body_size;
        std::string _server_name;
        std::string _root;
        std::string _index;
        std::vector<LocationConfig> _locations;
        std::map<int, std::string> _error_pages;

    public:
        ServerConfig();
        ~ServerConfig();

        void setPort(std::string port);
        void setServerName(std::string server_name);
        void setRoot(std::string root);
        void setIndex(std::string index_name);

        int getPort() const;
        const std::string& getServerName() const;
        const std::string& getRoot() const;
        const std::string& getIndex() const;



};

#endif
