#ifndef HTTPRESPONSE_HPP
#define HTTPRESPONSE_HPP

#include "./HelperFunctions.hpp"
#include <map>
#include <unistd.h>
#include <iostream>

class HTTPResponse {

	private: 
		int _statusCode;
		std::string _statusText;
		std::map<std::string, std::string> _headers;
		std::string _body;
		std::string _version;

	public:

		void setStatusCode(int code);
		void setHeader(const std::string& key, const std::string& value);
		void setStatus(const std::string& text);
		void setBody(const std::string& body);
		void setVersion(const std::string& version);
		
		std::string toString(HTTPResponse& ClassResponse) const;
		const std::string& getBody() const;
		const std::string& getVersion() const;
		
		// getStatusCode() is added only for TestMain.cpp 
		int getStatusCode() const { return _statusCode; }; 
};

#endif
