#include "./hpp/ConfigParser.hpp"
#include "./hpp/ServerConfig.hpp"

ConfigParser::ConfigParser() {
    setters["listen"] = &ServerConfig::setPort;
    setters["server_name"] = &ServerConfig::setServerName;
    setters["root"] = &ServerConfig::setRoot;
    setters["index"] = &ServerConfig::setIndex;
}

const std::vector<ServerConfig>& ConfigParser::getServers() const {

    return servers;
};


std::vector<std::string> tokenize(std::ifstream& file)
{
    std::vector<std::string> tokens;
    char c;
    std::string current;

    while (file.get(c))
    {
        if (std::isspace(static_cast<unsigned char>(c)))
        {
            if (!current.empty())
            {
                tokens.push_back(current);
                current.clear();
            }
        }
        else if (c == '{' || c == '}' || c == ';')
        {
            if (!current.empty())
            {
                tokens.push_back(current);
                current.clear();
            }

            tokens.push_back(std::string(1, c));
        }
        else
        {
            current += c;
        }
    }

    if (!current.empty())
        tokens.push_back(current);

    return tokens;
}

void ConfigParser::parse(const std::string& filename)
{

    std::ifstream file(filename.c_str());

    if (!file) {
        throw std::runtime_error("Cannot open config file");
	}
	std::vector<std::string> configTokens = tokenize(file);
    //check the brakets
    // MAP -> asdad array [1,1] =  ([]"listen", *setPort], ["server_name", *setServerName])
    for (size_t i = 0; i < configTokens.size(); i++) {

        if (configTokens[i] == "server" && i + 1 < configTokens.size() && configTokens[i + 1] == "{") {
            
            ServerConfig server;
            i += 2;
           
            while (i  < configTokens.size() && (configTokens[i] != "location" && configTokens[i] != "error_page")) {
                
                if (i + 2 >= configTokens.size())
                    throw std::runtime_error("Incomplete directive");
                
                std::string key = configTokens[i];
                std::string value = configTokens[i + 1];
                std::string semi = configTokens[i + 2];

                if (semi != ";")
                    throw std::runtime_error("Missing ; after " + key);
                
                std::map<std::string, Setter>::iterator it = setters.find(key);

                if (it == setters.end())
                    throw std::runtime_error("Unknown directive: " + key);

                Setter fun = it->second; // just Setter in ConfigParser is an alias to = void (ServerConfig::*)(const std::string&)
                (server.*fun)(value); // = we can call directly (server.*(it->second))(value);

                i += 3;
                
            }

            // while (i  < configTokens.size() && configTokens[i] == "error_page" ) {

            // }
            
            // while (i  < configTokens.size() && configTokens[i] == "location" ) {

            // }

            servers.push_back(server);
        }
    
    }

    // std::map<std::string, DirectiveParser>::iterator it =
    //     directiveMap.find(tokens[i]);

    // if (it == directiveMap.end())
    //     throw std::runtime_error("Unknown directive");

    // (this->*(it->second))(server, i);

    // check if all the values are there.
}

// const std::vector<ServerConfig>& ConfigParser::getServers() const {

// };


// ~~~~~~~~~~~~~~~~~~~ PRINT ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

	// std::cout << "Opened file: " << filename << std::endl;
	// std::string line;
    // while (std::getline(file, line))
    // {
    //     std::cout << line << std::endl;
    // }

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
	