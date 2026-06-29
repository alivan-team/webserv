#include "./hpp/ConfigParser.hpp"
#include "./hpp/ServerConfig.hpp"
#include "./hpp/LocationConfig.hpp"

ConfigParser::ConfigParser() {
    setters["listen"] = &ServerConfig::setPort;
    setters["server_name"] = &ServerConfig::setServerName;
    setters["root"] = &ServerConfig::setRoot;
    setters["index"] = &ServerConfig::setIndex;

	locationSetters["allow_methods"] = &LocationConfig::setAllowMethods;
	locationSetters["upload_store"] = &LocationConfig::setUploadStore;
	locationSetters["autoindex"] = &LocationConfig::setAutoIndex;
	locationSetters["root"] = &LocationConfig::setRoot;
	locationSetters["index"] = &LocationConfig::setIndex;
	locationSetters["cgi_extension"] = &LocationConfig::setCgiExtension;
	locationSetters["cgi_path"] = &LocationConfig::setCgiPath;
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
    for (size_t i = 0; i < configTokens.size(); i++) {

        if (configTokens[i] == "server" && i + 1 < configTokens.size() && configTokens[i + 1] == "{") {
            
            ServerConfig server;
            i += 2;
           
            while (i  < configTokens.size() && (configTokens[i] != "location" && configTokens[i] != "error_page")) {
                
                std::string key = configTokens[i];

                std::map<std::string, Setter>::iterator it = setters.find(key);
                // std::cout << "key -> " << key << "  <>  setters.find(key) -> " << it->first << std::endl;

                if (it == setters.end())
                    break ;

                i++;

                std::vector<std::string> values;     

                while (configTokens[i] != ";") {
                    values.push_back(configTokens[i]);
                    ++i;
                }

                if (i >= configTokens.size())
                    throw std::runtime_error("Missing ; after " + key);

                if (values.empty())
                    throw std::runtime_error("Missing value after " + key);
                    
                    
                Setter fun = it->second; // just Setter in ConfigParser is an alias to = void (ServerConfig::*)(const std::string&)
                (server.*fun)(values); // = we can call directly (server.*(it->second))(value);
                i++;
                    
            }
			
			while (i  < configTokens.size() && configTokens[i] == "location" ) {
				
				i++;
				std::string uripath = configTokens[i];
				LocationConfig location;

				if (i + 2  >= configTokens.size() || configTokens[i + 1] != "{"){
					throw std::runtime_error("Missing location sectin");
				}

				i++;
				while (i  < configTokens.size() && configTokens[i] != "}"){

					std::string key = configTokens[i];

					std::map<std::string, LocationSetter>::iterator it = locationSetters.find(key);

					if (it == locationSetters.end())
						break ;

					i++;

					std::vector<std::string> values;     

					while (i  < configTokens.size() && configTokens[i] != ";") {
						values.push_back(configTokens[i]);
						++i;
					}
					if (i >= configTokens.size())
						throw std::runtime_error("Missing ; after " + key);

					if (values.empty())
						throw std::runtime_error("Missing value after " + key);

					LocationSetter fun = it->second; // just Setter in ConfigParser is an alias to = void (ServerConfig::*)(const std::string&)
					(location.*fun)(values); // = we can call directly (server.*(it->second))(value);
					i++;
				}
				location.setUriPath(uripath);
				server.addLocation(location);
				i++;
			}
			
            // while (i  < configTokens.size() && configTokens[i] == "error_page" ) {

            // }

            // check if the current token has "}" 
            servers.push_back(server);
        }
    
    }

    // bool valid(i) {
    //     if (i + 1 valid)
    //         true
    //     false
    // }

    // while (configTokens[i] != "}")
    //     {
    //         if (valid && configTokens[i] == "listen")
    //         {
    //             server.setPort(configTokens[i + 1]);

    //         }
    //         else if (configTokens[i] == "server_name")
    //         {
    //             server.setServerName(configTokens[i + 1]);
    //         }
    //         else if (configTokens[i] == "root")
    //         {
    //             server.setRoot(configTokens[i + 1]);
    //         }
    //         else if (configTokens[i] == "index")
    //         {
    //             server.setIndex(configTokens[i + 1]);
    //         }
    //         if (configTokens[i] == "location")
    //             // call funcitons for location .

    //     }



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
	