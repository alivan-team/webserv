#include "./hpp/ConfigParser.hpp"

ConfigParser::ConfigParser() {
    setters["listen"] = &ServerConfig::setPort;
    setters["server_name"] = &ServerConfig::setServerName;
    setters["root"] = &ServerConfig::setRoot;
    setters["index"] = &ServerConfig::setIndex;
}

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
    // MAP -> asdad array [1,1] =  ([]"listen", *setPort], ["server_name", *setServerName])
    for (int i = 0; i < configTokens.size(); i++) {

        if (configTokens[i] == "server" && i + 1 < configTokens.size() && configTokens[i + 1] == "{") {
            
            ServerConfig server;
            i += 2;
           
            while (i  < configTokens.size() && configTokens[i] != "}" ) {
                
                try {

                } catch(std::string exeption ) {
                    throw ;
                }
            }

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
	