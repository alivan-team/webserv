#ifndef HTTP_REQUEST_HPP
#define HTTP_REQUEST_HPP

#include <exception>
#include <string>
#include <map>
#include <cctype>
#include "externalStructures.hpp"

class HTTPRequest{

	private:
		Method _method;
		std::string _uri;
		std::string _path;
		std::string _query;
		std::string _version;
		std::map<std::string, std::string> _headers;
		std::string _body;

	public:
		HTTPRequest();
		~HTTPRequest() = default;

		const Method &getMethod() const;
		const std::string &getUri() const;
		const std::string &getPath() const;
		const std::string &getQuery() const;
		const std::string &getVersion() const;
		const std::string &getBody() const;

		bool hasHeader(const std::string &name) const;
		const std::string &getHeader(const std::string &name) const;
};

#endif