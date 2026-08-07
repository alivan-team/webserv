#ifndef HELPERFUNCTIONS_HPP
#define HELPERFUNCTIONS_HPP

#include <string>

bool check_num(const std::string& value);
bool checkUriPath(const std::string& uripath);
bool checkFSPath(const std::string &fspath);
bool hasControlChar(const std::string& s);
std::string trim(const std::string& value);
std::string toLower(const std::string& value);

#endif
