#include "./hpp/ConfigParser.hpp"
#include "./hpp/ServerConfig.hpp"
#include "./hpp/LocationConfig.hpp"

ConfigParser::ConfigParser() {
    setters["listen"] = &ServerConfig::setPort;
    setters["server_name"] = &ServerConfig::setServerName;
    setters["root"] = &ServerConfig::setRoot;
    setters["index"] = &ServerConfig::setIndex;
    setters["client_max_body_size"] = &ServerConfig::setClientMaxBodySize;
    setters["error_page"] = &ServerConfig::setErrorPage;

	// locationSetters["allow_methods"] = &LocationConfig::setAllowMethods;
	// locationSetters["upload_store"] = &LocationConfig::setUploadStore;
	// locationSetters["autoindex"] = &LocationConfig::setAutoIndex;
	// locationSetters["root"] = &LocationConfig::setRoot;
	// locationSetters["index"] = &LocationConfig::setIndex;
	// locationSetters["cgi_extension"] = &LocationConfig::setCgiExtension;
	// locationSetters["cgi_path"] = &LocationConfig::setCgiPath;
	locationSetters["return"] = &LocationConfig::setRedirect;
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
        if (c == '#')
        {
            while (file.get(c) && c != '\n' && c != '\r') {};
            continue;
        }
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
    // check currect file

    if (!file) {
        throw std::runtime_error("Cannot open config file");
	}
	std::vector<std::string> configTokens = tokenize(file);
    //check the brakets
    // MAP -> asdad array [1,1] =  ([]"listen", *setPort], ["server_name", *setServerName])
    // for (auto t : configTokens) {
    //     std::cout << "configTokens[i] -> " << t << std::endl;
    // }

    for (size_t i = 0; i < configTokens.size(); i++) {

        if (configTokens[i] == "server" && i + 1 < configTokens.size() && configTokens[i + 1] == "{") {
            
            ServerConfig server;
            i += 2;
            // std::cout << "configTokens[i] " << configTokens[i] << std::endl;
            while (i < configTokens.size() && configTokens[i] != "}") {


                if (configTokens[i] == "location") {
                    // std::cout << "location -> HI " << std::endl;

                    i++;
                    std::string uripath = configTokens[i];
                    LocationConfig location;

                    if (i + 2  >= configTokens.size() || configTokens[i + 1] != "{"){
                        throw std::runtime_error("Missing location sectin");
                    }

                    i++;
                    while (i  < configTokens.size() && configTokens[i] != "}"){
                        findKey(location, locationSetters, configTokens, i);
                    }
                    location.setUriPath(uripath);
                    server.addLocation(location);
                    i++;
                } else { // process server information and error_pages
                    findKey(server, setters, configTokens, i);
                }

            }

            // check if the current token has "}" 
            servers.push_back(server);
        }
    
    }


// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

	// ServerConfig server;

    // read file
    // fill ServerConfig

	// put the parsed object in the vector.

    // return config;
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
	