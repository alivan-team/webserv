#include "./hpp/HTTPRequest.hpp"

		HTTPRequest::HTTPRequest()
			: _method(Method::UNKNOWN) { };

		void HTTPRequest::setMethod(Method method){ _method = method; };

		void HTTPRequest::setUri(const std::string &uri){ _uri = uri; };

		void HTTPRequest::setPath(const std::string &path){ _path = path; };

		void HTTPRequest::setQuery(const std::string &query){ _query = query; };

		void HTTPRequest::setVersion(const std::string &version){ _version = version;};

		void HTTPRequest::addHeader(const std::string &name, const std::string &value){
			_headers[name] = value;
		};

		Method HTTPRequest::getMethod() const { return _method; };

		const std::string& HTTPRequest::getUri() const{ return _uri; };
		const std::string& HTTPRequest::getPath() const{ return _path; };
		const std::string& HTTPRequest::getQuery() const{ return _query; };
		const std::string& HTTPRequest::getVersion() const{ return _version; };
		const std::map<std::string, std::string>& HTTPRequest::getHeaders() const{ return _headers; };

		bool HTTPRequest::hasHeader(const std::string &name) const{ return (_headers.find(name) != _headers.end()); };
		const std::string& HTTPRequest::getHeader(const std::string &name) const{
			std::map<std::string, std::string>::const_iterator it;

			it = _headers.find(name);

			if (it == _headers.end())
				throw std::runtime_error("Header not found");

    		return (it->second); 
		};