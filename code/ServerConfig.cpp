#include "./hpp/ServerConfig.hpp"
#include "./hpp/client.hpp"

//	LOCATION CONFIG

//  SERVER CONFIG 

ServerConfig::ServerConfig() : _port({8080}), _server_name({"localhost"}), _root({"./www"}), _index({"index.html"}) {};

ServerConfig::~ServerConfig() {};

static bool hasControlChar(const std::string& s)
{
    for (char ch : s) {
        if (std::iscntrl(static_cast<unsigned char>(ch)))
            return true;
    }
    return false;
}

static bool validServerNameChar(char ch)
{
    unsigned char c = static_cast<unsigned char>(ch);
    return std::isalnum(c) || c == '_' || c == '-' || c == '.';
}

static bool validServerName(const std::string& name)
{
    if (name.empty())
        return false;

    for (char ch : name) {
        if (!validServerNameChar(ch))
            return false;
    }
    return true;
}


void ServerConfig::setPort(const std::vector<std::string>& port){

	// _port.pop_back();
	// std::cout << "Setport -> " << port.size() << "post var --> " << port[0] << port[1] << std::endl;
	if (port.size() != 1 || !check_num(port[0]))
		throw std::runtime_error("Incorrect port");
    
	int i_port = std::atoi(port[0].c_str());

	if (i_port < 1 || i_port > 65535) {
		throw std::runtime_error("Invalid port");
	}

	_port.push_back(i_port);
};

void ServerConfig::setServerName(const std::vector<std::string>& server_name) {

	if (server_name.empty())
        throw std::runtime_error("Invalid server name");

	for (const std::string& name : server_name) {
        if (!validServerName(name))
            throw std::runtime_error("Invalid server name");
    }

    _server_name.insert(_server_name.end(), server_name.begin(), server_name.end());
};

void ServerConfig::setRoot(const std::vector<std::string>& root) {

	if (root.empty() || root.size() != 1)
    	throw std::runtime_error("Invalid root");

	for (const std::string& r : root) {

		if (r.empty())
    			throw std::runtime_error("Invalid root");

		if (hasControlChar(r)) {
			throw std::runtime_error("Invalid root");
		}
	}
	_root.insert(_root.end(), root.begin(), root.end());

};

void ServerConfig::setIndex(const std::vector<std::string>& index_name) {

	if (index_name.empty())
    	throw std::runtime_error("Invalid index name");

	for (const std::string& index : index_name) {

		if (index.empty() || index.find('/') != std::string::npos || hasControlChar(index))
			throw std::runtime_error("Invalid index name");
	}
		
	_index.insert(_index.end(), index_name.begin(), index_name.end());

};



const std::vector<int>& ServerConfig::getPort() const { return _port; };

const std::vector<std::string>& ServerConfig::getServerName() const { return _server_name; };

const std::vector<std::string>& ServerConfig::getRoot() const { return _root; };

const std::vector<std::string>& ServerConfig::getIndex() const { return _index; };


// Added by Alina for Location block
void ServerConfig::addLocation(const LocationConfig &locations) {
		
	_locations.push_back(locations);

};
