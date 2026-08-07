#ifndef HTTP_REQUEST_HPP
#define HTTP_REQUEST_HPP

#include <string>
#include <map>
#include <stdexcept>
#include <cctype>
#include "externalStructures.hpp"

class HTTPRequest{

	private:
		Method _method;
		std::string _uri;
		std::string _path;
		std::string _query;
		std::string _version;
		std::map<std::string, std::string> _headers; // "key" -> "value";
		const std::string* _requestBuffer;
		size_t _bodyOffset;
		size_t _bodySize;

	public:
		HTTPRequest();
		~HTTPRequest() = default;

		void setMethod(Method method);
		void setUri(const std::string &uri);
		void setPath(const std::string &path);
		void setQuery(const std::string &query);
		void setVersion(const std::string &version);
		void addHeader(const std::string &name, const std::string &value);
		void setBodyLocation(const std::string& requestBuffer, size_t offset, size_t size);
		
		Method getMethod() const;
		const std::string &getUri() const;
		const std::string &getPath() const;
		const std::string &getQuery() const;
		const std::string &getVersion() const;
		const std::map<std::string, std::string> &getHeaders() const;
		const std::string& getRequestBuffer() const;
		size_t getBodyOffset() const;
		size_t getBodySize() const;

		bool hasHeader(const std::string &name) const;
		const std::string &getHeader(const std::string &name) const;
};

#endif
