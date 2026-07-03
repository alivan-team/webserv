#ifndef HTTP_REQUEST_PARSER_HPP
#define HTTP_REQUEST_PARSER_HPP

#include "externalStructures.hpp"
#include "HTTPRequest.hpp"

class HTTPRequestParser{
	public:
		HTTPRequest parse(std::string &buffer) const;
};

#endif