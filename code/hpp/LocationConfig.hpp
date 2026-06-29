#ifndef LOCATION_CONFIG_HPP
#define LOCATION_CONFIG_HPP

#include <iostream>
#include <vector>
#include <exception>
#include <string>
#include <map>

struct Method{
	bool get;
	bool post;
	bool del;
};

class LocationConfig {

    private:
        std::string _uriPath;                  // "/upload" -> CGI
        Method methods;  // GET POST DELETE
        std::string upload_store;

    public:
        LocationConfig();
        ~LocationConfig();

        // evaluations for Location cofig
        bool checkUriPath(std::string uripath);
        bool setUriPath(std::string uripath);
        bool getUriPath(std::string uripath);

};

#endif
