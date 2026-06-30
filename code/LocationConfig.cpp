#include "./hpp/LocationConfig.hpp"
#include "./hpp/client.hpp"

LocationConfig::LocationConfig()
    : _uriPath(), _methods(), _upload_store()
{
    _methods.get = false;
    _methods.post = false;
    _methods.del = false;
}

LocationConfig::~LocationConfig()
{
}



void LocationConfig::setUriPath(std::string uripath)
{
    if (!checkUriPath(uripath))
        throw std::runtime_error("Incorrect URI path");
    _uriPath = uripath;
}

void LocationConfig::setAllowMethods(const std::vector<std::string>& methods){

	if (methods.size() == 0)
		throw std::runtime_error("Incorrect Allow methods in configuration file");
	
	for (size_t i = 0; i < methods.size(); ++i)
	{
	    if (methods[i] == "GET")
	        _methods.get = true;
	    else if (methods[i] == "POST")
	        _methods.post = true;
	    else if (methods[i] == "DELETE")
	        _methods.del = true;
	    else
	        throw std::runtime_error("Unknown Allow methods in configuration file");
}

};


void LocationConfig::setUploadStore(const std::vector<std::string>& fspath){
	
	if (fspath.size() != 1 || checkFSPath(fspath[0]))
		throw std::runtime_error("Incorrect Upload store in configuration file");
		
	_upload_store = fspath[0];
};

void LocationConfig::setAutoIndex(const std::vector<std::string>& indexes){

	if (indexes.size() != 1 || checkFSPath(indexes[0]))
		throw std::runtime_error("Incorrect AutoIndex in configuration file");
		
	_autoIndex = indexes[0] == "on";
};

void LocationConfig::setRoot(const std::vector<std::string>& fspath){

		if (fspath.size() != 1 || checkFSPath(fspath[0]))
		throw std::runtime_error("Incorrect root in location");
		
	_rootPath = fspath[0];
};

void LocationConfig::setIndex(const std::vector<std::string>& indpaths){

};

// void LocationConfig::setCgiExtension(const std::vector<std::string>& cgiexs){

// };

// void LocationConfig::setCgiPath(const std::vector<std::string>& cgipath){

// };

void LocationConfig::setRedirect(const std::vector<std::string>& redirpath){
	if (redirpath.size() != 2)
		throw std::runtime_error("Incorrect argument number for Redirection");

	if (!check_num(redirpath[0]) || checkFSPath(redirpath[1]))
		throw std::runtime_error("Incorrect argument value for Redirection");
	_redir._number = std::stoi(redirpath[0]);
	_redir._redirPath = redirpath[1];
};

std::string LocationConfig::getUriPath()
{
    return _uriPath;
}
