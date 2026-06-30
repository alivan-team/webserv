#include "./hpp/client.hpp"

bool check_num(const std::string& value) { 

	if (value.empty())
        return false;

    for (size_t i = 0; i < value.size(); i++) {
        if (!std::isdigit(static_cast<unsigned char>(value[i])))
            return false;
    }
	return true;
}

bool checkUriPath(const std::string& uripath)
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

bool checkFSPath(const std::string &fspath)
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