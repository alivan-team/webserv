#ifndef HTTPRESPONSEBUILD_HPP
#define HTTPRESPONSEBUILD_HPP

#include "./HTTPResponse.hpp"
#include "./HTTPRequest.hpp"
#include "./ServerConfig.hpp"
#include "./LocationConfig.hpp"
#include <fstream>
#include <sys/stat.h>
#include <unistd.h>
#include <iostream>
#include <vector>
#include <map>
#include <string>
#include <fstream>
#include <sstream>
#include <algorithm>
#include <dirent.h>

class HTTPResponseBuild {

    private: 
        static HTTPResponse makeErrorResponse(int code, const HTTPRequest& request, const ServerConfig& servConf);

        static HTTPResponse handleGet(const HTTPRequest& request, const ServerConfig& servConf);

        static std::string getStatusText(int code);
        static std::string decideConnection(const HTTPRequest& request);
        static std::string  buildErrorBody(int code, const ServerConfig& servConf);
        static std::string joinPath(const std::string& root, const std::string& path);
        static bool fileExists(const std::string& file);
        static bool canReadFile(const std::string& file);
        static std::string readReadFile(const std::string& file);
        static bool isDirectory(const std::string& path);
        
        static const LocationConfig* findBestLocation (const std::string& path,  const ServerConfig& servConf);
        static std::string findIndexFile(std::string fullPath, const LocationConfig& location, const ServerConfig& servConf);
        static bool startsWithLocation(const std::string& path, const std::string& loc);

        static HTTPResponse buildAutoIndexPage(const HTTPRequest& request, const ServerConfig& servConf, const std::string& directoryPath, const std::string& requestPath);
        static bool checkExtensionOfFile(const std::string& extension);

	public:
    	static HTTPResponse build(const HTTPRequest& request, const ServerConfig& servConf);
};

#endif


    // std::string response =
    // "HTTP/1.1 200 OK\r\n"
    // "Content-Type: text/plain\r\n"
    // "Content-Length: " + std::to_string(body.size()) + "\r\n"
    // "\r\n" +
    // body;