#include "./hpp/HTTPRequestParser.hpp"

	void HTTPRequestParser::parseRequestLine(){

	};

	void HTTPRequestParser::parseHeaders(){

	};

	void HTTPRequestParser::parseUri(){

	};

	Method HTTPRequestParser::parseMethod(std::string methods){

		if (methods == "GET")
			return Method::GET;
		else if (methods == "POST")
			return Method::POST;
		else if (methods == "DELETE")
			return Method::DELETE;
		else
			return Method::UNKNOWN;
	};

	// HTTPRequest HTTPRequestParser::parse(const std::string &buffer) const{

	// };