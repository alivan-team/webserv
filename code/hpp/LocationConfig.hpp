#ifndef LOCATION_CONFIG_HPP
#define LOCATION_CONFIG_HPP

#include <iostream>
#include <vector>
#include <exception>
#include <string>
#include <map>
#include <cctype>
#include <algorithm>

struct Method{
	bool get;
	bool post;
	bool del;
};

struct Redirection{
	int	_number; // status 
	std::string _redirPath; // - redirect for this error
};
class LocationConfig {

    private:
        std::string _uriPath;                  // "/upload" -> CGI
        Method _methods;  // GET POST DELETE
        std::string _upload_store;
		// bool _autoIndex;
		std::string _rootPath;
		bool _autoIndex;
		std::vector<std::string> _indpaths;
		std::vector<std::string> _cgi_extensions;
		std::vector<std::string> _cgi_paths;
		Redirection _redir;

		// evaluations for Location config
		bool checkUriPath(const std::string &uripath);
		bool checkFSPath(const std::string &fspath);

    public:
        LocationConfig();
        ~LocationConfig();

		// settings Location parameters
        void setUriPath(std::string uripath);
		void setAllowMethods(const std::vector<std::string>& methods);
		void setUploadStore(const std::vector<std::string>& fspath);
		void setAutoIndex(const std::vector<std::string>& indexes); // Index from Location can override Index from server
		void setRoot(const std::vector<std::string>& fspath);
		void setIndex(const std::vector<std::string>& indpaths);
		// void setCgiExtension(const std::vector<std::string>& cgiexs);
		// void setCgiPath(const std::vector<std::string>& cgipath);
		void setRedirect(const std::vector<std::string>& redirpath);
		
		// gettings Location parameters
		// bool getAllowMethods();
		// bool getAutoIndex();
        std::string getUriPath();
		// std::string getUploadStore();
		// std::string getRoot();
		// std::string getIndex();
		// std::string getCgiExtension();
		// std::string getCgiPath();
		// std::string getRedirect();
};

#endif
