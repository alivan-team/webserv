#ifndef LOCATION_CONFIG_HPP
#define LOCATION_CONFIG_HPP

#include <iostream>
#include <vector>
#include <exception>
#include <string>
#include <map>
#include <cctype>
#include <algorithm>

#include "printDebug.hpp"
#include "externalStructures.hpp"

class LocationConfig {

	private:
		std::string _uriPath;
		AllowMethods _methods;
		std::string _upload_store;
		std::string _rootPath;
		bool _autoIndex;
		std::vector<std::string> _indpaths;
		std::vector<std::string> _cgi_extensions;
		std::vector<std::string> _cgi_paths;
		Redirection _redir;
		unsigned int _clientMaxBodySize;
		bool _hasClientMaxBodySize;

	public:
		LocationConfig();
		~LocationConfig();

		void setUriPath(std::string uripath);
		void setAllowMethods(const std::vector<std::string>& methods);
		void setUploadStore(const std::vector<std::string>& fspath);
		void setAutoIndex(const std::vector<std::string>& indexes);
		void setRoot(const std::vector<std::string>& fspath);
		void setIndex(const std::vector<std::string>& indpaths);
		void setCgiExtension(const std::vector<std::string>& cgiexs);
		void validateCgiConfig() const;
		void setCgiPath(const std::vector<std::string>& cgipath);
		void setRedirect(const std::vector<std::string>& redirpath);
		
		AllowMethods getAllowMethods() const;
		bool getAutoIndex() const;
		bool hasRedirect() const;
		const std::string& getUriPath() const;
		const std::string& getUploadStore() const;
		const std::string& getRoot() const;
		const std::vector<std::string>& getIndex() const;
		const std::vector<std::string>& getCgiPath() const;
		const std::vector<std::string>& getCgiExtension() const;
		Redirection getRedirect() const;
		bool isGetAllowed() const;
		bool isPostAllowed() const;
		bool isDeleteAllowed() const;
		void setClientMaxBodySize(const std::vector<std::string>& client_max_body_size);
		unsigned int getClientMaxBodySize() const;
		bool hasClientMaxBodySize() const;
};

#endif
