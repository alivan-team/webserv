#include "./hpp/LocationConfig.hpp"
#include "./hpp/client.hpp"

LocationConfig::LocationConfig()
    : _uriPath(),
      _methods(),
      _upload_store(),
      _rootPath(),
      _autoIndex(false),
      _indpaths(),
      _cgi_extensions(),
      _cgi_paths(),
      _redir()
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
	// printDebug("\n_uriPath: ", _uriPath);
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
	// printDebug("_methods.get > ", _methods.get);
	// printDebug("_methods.post > ", _methods.post);
	// printDebug("_methods.del > ", _methods.del);
};

void LocationConfig::setUploadStore(const std::vector<std::string>& fspath){
	
	if (fspath.size() != 1 || !checkFSPath(fspath[0]))
		throw std::runtime_error("Incorrect Upload store in configuration file");
		
	_upload_store = fspath[0];
	// printDebug("_upload_store > ", _upload_store);
};

void LocationConfig::setAutoIndex(const std::vector<std::string>& indexes){

	if (indexes.size() != 1 || !checkFSPath(indexes[0]))
		throw std::runtime_error("Incorrect AutoIndex in configuration file");
		
	_autoIndex = indexes[0] == "on";
	// printDebug("_autoIndex > ", _autoIndex);
};

void LocationConfig::setRoot(const std::vector<std::string>& fspath){

		if (fspath.size() != 1 || !checkFSPath(fspath[0]))
		throw std::runtime_error("Incorrect root in location");
		
	_rootPath = fspath[0];
	// printDebug("_rootPath > ", _rootPath);
};

void LocationConfig::setIndex(const std::vector<std::string>& indpaths){

	if (indpaths.empty())
    	throw std::runtime_error("Invalid index name in location");

	for (const std::string& index : indpaths) {

		if (index.empty() || !checkFSPath(index))
			throw std::runtime_error("Invalid index name in location");
	}
		
	_indpaths.insert(_indpaths.end(), indpaths.begin(), indpaths.end());
	// printDebug("_indexes: ", _indpaths);
};

void LocationConfig::setCgiExtension(const std::vector<std::string>& cgiexs){
	// Validate each extension
	for (const std::string& ext : cgiexs) {
		if (ext.empty() || ext[0] != '.' || ext.find('/') != std::string::npos || ext.find(' ') != std::string::npos)
			throw std::runtime_error("Invalid CGI extension in configuration file");
	}
	_cgi_extensions = cgiexs;
	// printDebug("_cgi_extensions > ", _cgi_extensions);
	if (!_cgi_paths.empty() && _cgi_paths.size() != _cgi_extensions.size())
		throw std::runtime_error("CGI extensions and CGI paths count mismatch");
}

void LocationConfig::setCgiPath(const std::vector<std::string>& cgipath)
{
	if (cgipath.empty())
		throw std::runtime_error("Incorrect CGI path in configuration file");

	if (!_cgi_extensions.empty() && cgipath.size() != _cgi_extensions.size())
		throw std::runtime_error("CGI extensions and CGI paths count mismatch");

	for (const std::string& path : cgipath)
	{
		if (!checkFSPath(path))
			throw std::runtime_error("Incorrect CGI path in configuration file");
		_cgi_paths.push_back(path);
	}
	// printDebug("_cgi_paths > ", _cgi_paths);
}

void LocationConfig::setRedirect(const std::vector<std::string>& redirpath){
	if (redirpath.size() != 2)
		throw std::runtime_error("Incorrect argument number for Redirection");

	if (!check_num(redirpath[0]) || !checkFSPath(redirpath[1]))
		throw std::runtime_error("Incorrect argument value for Redirection");
	_redir._number = std::stoi(redirpath[0]);
	_redir._redirPath = redirpath[1];
	// printDebug("_redir._number > ", _redir._number);
	// printDebug("_redir._redirPath > ", _redir._redirPath);
};

const std::string& LocationConfig::getUriPath() const
{
    return _uriPath;
}

AllowMethods LocationConfig::getAllowMethods() const
{
	return _methods;
}

bool LocationConfig::getAutoIndex() const
{
	return _autoIndex;
}

bool LocationConfig::hasRedirect() const
{
	return _redir._number != 0;
}

const std::string& LocationConfig::getUploadStore() const
{
	return _upload_store;
}

const std::string& LocationConfig::getRoot() const
{
	return _rootPath;
}

const std::vector<std::string>& LocationConfig::getIndex() const
{
	return _indpaths;
}

const std::vector<std::string>& LocationConfig::getCgiExtension() const
{
	return _cgi_extensions;
}

Redirection LocationConfig::getRedirect() const
{
	return _redir;
}

bool LocationConfig::isGetAllowed() const {
	return _methods.get;
}

bool LocationConfig::isPostAllowed() const {
	return _methods.post;
}

bool LocationConfig::isDeleteAllowed() const {
	return _methods.del;
}