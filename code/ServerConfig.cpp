#include "./hpp/ServerConfig.hpp"
#include "./hpp/client.hpp"

//	LOCATION CONFIG

//  SERVER CONFIG 

ServerConfig::ServerConfig() : _port({8080}), _server_name({"localhost"}), _root({"./www"}), _index({"index.html"}), _client_max_body_size({1000000}) {};

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

void ServerConfig::setClientMaxBodySize(const std::vector<std::string>& client_max_body_size) {

	// char *end;
	errno = 0;

	if (client_max_body_size.size() != 1)
		throw std::runtime_error("client_max_body_size expects exactly one value");
	
	if (!check_num(client_max_body_size[0])) {
		throw std::runtime_error("Invalid client_max_body_size");
	}

	unsigned long long int value = std::strtoull(client_max_body_size[0].c_str(), NULL, 10);

	if (errno == ERANGE || value > UINT_MAX)
		throw std::runtime_error("Invalid client_max_body_size number");

	_client_max_body_size.push_back(static_cast<unsigned>(value));
};

void ServerConfig::setErrorPage(const std::vector<std::string>& error_page) {

	if (error_page.size() < 2)
		throw std::runtime_error("error_page expects code(s) and path");

		std::string path = error_page.back();

		for (size_t i = 0; i + 1 < error_page.size(); i++)
		{
			if (!check_num(error_page[i]))
				throw std::runtime_error("Invalid error_page code");

			int code = std::atoi(error_page[i].c_str());

			if (code < 300 || code > 599)
				throw std::runtime_error("Invalid error_page code");

			_error_pages[code] = path;
		}

};




const std::vector<int>& ServerConfig::getPort() const { return _port; };

const std::vector<std::string>& ServerConfig::getServerName() const { return _server_name; };

const std::vector<std::string>& ServerConfig::getRoot() const { return _root; };

const std::vector<std::string>& ServerConfig::getIndex() const { return _index; };

const std::vector<unsigned int>& ServerConfig::getClientMaxBodySize() const { return _client_max_body_size; };

const std::map<int, std::string>& ServerConfig::getErrorPage() const { return _error_pages; };



// Added by Alina for Location block
void ServerConfig::addLocation(const LocationConfig &locations) {
		
	_locations.push_back(locations);
};
