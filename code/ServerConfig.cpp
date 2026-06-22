#include "./hpp/ServerConfig.hpp"

//	LOCATION CONFIG

//  SERVER CONFIG 

ServerConfig::ServerConfig() : _port(8080), _server_name("localhost"), _root("./www"), _index("index.html") {};

ServerConfig::~ServerConfig() {};

void ServerConfig::setPort(int port){

	if (port < 1 || port > 65535) {
		throw std::runtime_error("Invalid port");
	}

	_port = port;
};

void ServerConfig::setServerName(std::string server_name) {

	if (server_name.empty())
        throw std::runtime_error("Invalid server name");

    for (size_t i = 0; i < server_name.size(); i++)
    {
        unsigned char c = static_cast<unsigned char>(server_name[i]);

        if (!std::isalnum(c) && c != '_' && c != '-' && c != '.')
            throw std::runtime_error("Invalid server name");
    }

	_server_name = server_name;
};

void ServerConfig::setRoot(std::string root) {

	if (root.empty())
    	throw std::runtime_error("Invalid root");
	
	for (size_t i = 0; i < root.size(); i++)
	{
		if (std::iscntrl(root[i]))
			throw std::runtime_error("Invalid root");
	}

	_root = root;
};

void ServerConfig::setIndex(std::string index_name) {

	if (index_name.empty())
    	throw std::runtime_error("Invalid index name");

	if (index_name.find('/') != std::string::npos)
		throw std::runtime_error("Invalid index name");

	for (size_t i = 0; i < index_name.size(); i++)
	{
		if (std::iscntrl(index_name[i]))
			throw std::runtime_error("Invalid index name");
	}
	
	_index = index_name;
};

int ServerConfig::getPort() const { return _port; };

const std::string& ServerConfig::getServerName() const { return _server_name; };

const std::string& ServerConfig::getRoot() const { return _root; };

const std::string& ServerConfig::getIndex() const { return _index; };
