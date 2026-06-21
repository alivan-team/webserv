class LocationConfig
{
public:
    std::string path;                  // "/upload"
    std::vector<std::string> methods;  // GET POST DELETE
    std::string upload_store;
};

class ServerConfig
{
public:
    int port;
    std::string server_name;
    std::string root;
    std::string index;

    std::vector<LocationConfig> locations;
};

int main(int argc, char **argv)
{
    if (argc != 2)
    {
        std::cerr << "Usage: ./webserv <config_file>\n";
        return 1;
    }

    Config config;

    try
    {
        config = ConfigParser::parse(argv[1]);
    }
    catch (const std::exception& e)
    {
        std::cerr << "Config error: " << e.what() << "\n";
        return 1;
    }

    // use config.servers[0].port for socket/bind/listen
    // then use config later when handling requests
}