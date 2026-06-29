#include "./hpp/LocationConfig.hpp"

#include <cctype>

LocationConfig::LocationConfig()
    : _uriPath(), methods(), upload_store()
{
    methods.get = false;
    methods.post = false;
    methods.del = false;
}

LocationConfig::~LocationConfig()
{
}

bool LocationConfig::checkUriPath(std::string uripath)
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

bool LocationConfig::setUriPath(std::string uripath)
{
    if (!checkUriPath(uripath))
        return false;
    _uriPath = uripath;
    return true;
}

bool LocationConfig::getUriPath(std::string uripath)
{
    return _uriPath == uripath;
}
