#ifndef HTTPRESPONSEBUILD_HPP
#define HTTPRESPONSEBUILD_HPP

#include "./HTTPResponse.hpp"
#include "./HTTPRequest.hpp"
#include "./ServerConfig.hpp"
#include "./LocationConfig.hpp"

class HTTPResponseBuild {

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