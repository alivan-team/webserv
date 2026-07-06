#ifndef HTTP_REQUEST_PARSER_HPP
#define HTTP_REQUEST_PARSER_HPP

#include "externalStructures.hpp"
#include "HTTPRequest.hpp"

class HTTPRequestParser{
	private:
		void parseRequestLine();
		void parseHeaders();
		void parseBody(const std::string &body, HTTPRequest &request) const;
		void parseUri();

		Method parseMethod(...);

	public:
		HTTPRequest parse(const std::string &buffer) const;
};

#endif