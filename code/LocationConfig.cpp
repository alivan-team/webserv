#include "./hpp/LocationConfig.hpp"
#include "./hpp/client.hpp"

#include <cctype>

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

bool LocationConfig::checkUriPath(const std::string& uripath)
{
    if (uripath.empty() || uripath[0] != '/')
        return false;

    for (std::string::size_type i = 0; i < uripath.size(); ++i) {
        unsigned char ch = static_cast<unsigned char>(uripath[i]);
        if (std::isspace(ch) || std::iscntrl(ch))
            return false;
    }
    return true;
}

bool LocationConfig::checkFSPath(const std::string &fspath)
{
    if (fspath.empty())
        return false;

    for (std::string::size_type i = 0; i < fspath.size(); ++i) {
        unsigned char ch = static_cast<unsigned char>(fspath[i]);
        if (std::isspace(ch) || std::iscntrl(ch))
            return false;
    }
    return true;
}

void LocationConfig::setUriPath(std::string uripath)
{
    if (!checkUriPath(uripath))
        throw std::runtime_error("Incorrect URI path");
    _uriPath = uripath;
}

// void LocationConfig::setAllowMethods(const std::vector<std::string>& methods){

// };

// void LocationConfig::setUploadStore(const std::vector<std::string>& fspath){

// };

// void LocationConfig::setAutoIndex(const std::vector<std::string>& indexes){

// };

// void LocationConfig::setRoot(const std::vector<std::string>& fspath){

// };

// void LocationConfig::setIndex(const std::vector<std::string>& indpaths){

// };

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
