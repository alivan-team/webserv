#ifndef LOCATION_CONFIG_HPP
#define LOCATION_CONFIG_HPP

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
        LocationConfig();
        ~LocationConfig();
		
        // evaluations for Location cofig
        bool checkPath(std::string path);
        bool setPath(std::string path);
        bool getPath(std::string path);

        
};

#endif