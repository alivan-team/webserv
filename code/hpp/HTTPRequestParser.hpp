#ifndef HTTP_REQUEST_PARSER_HPP
#define HTTP_REQUEST_PARSER_HPP

#include "externalStructures.hpp"
#include "HTTPRequest.hpp"
#include <string>

class HTTPRequestParser{
	private:
		void parseHeaders(
		    const std::string &headers,
		    HTTPRequest &request) const;
		void parseUri(
		    const std::string &uri,
		    HTTPRequest &request) const;

		Method parseMethod(const std::string &method) const;
		std::string trim(const std::string &text) const;

	public:
		HTTPRequest parse(const std::string &buffer) const;
		void parseRequestLine(
			const std::string &line, 
			HTTPRequest &request) const;
};

#endif