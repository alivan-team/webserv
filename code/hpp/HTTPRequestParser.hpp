#ifndef HTTP_REQUEST_PARSER_HPP
#define HTTP_REQUEST_PARSER_HPP

#include "externalStructures.hpp"
#include "HTTPRequest.hpp"

class HTTPRequestParser{
	private:
		void parseRequestLine();
		void parseHeaders();
		void parseUri();

		Method parseMethod(std::string methods);

	public:
		HTTPRequest parse(const std::string &buffer) const;
};

#endif