#ifndef PRINT_DEBUG_HPP
#define PRINT_DEBUG_HPP

#include <iostream>
#include <map>
#include <exception>

class LocationConfig;

void printDebug(std::string title, const bool& value);
void printDebug(std::string title, const int& value);
void printDebug(std::string title, const size_t& value);
void printDebug(std::string title, const std::string& str);
void printDebug(std::string title, const std::vector<int>& vec);
void printDebug(std::string title, const std::vector<unsigned int>& vec);
void printDebug(std::string title, const std::vector<std::string>& vec);
void printDebug(std::string title, const std::map<int, std::string>& vec);
void printDebug(std::string title, const LocationConfig& location);

#endif