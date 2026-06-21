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

int main() {

    return 0;
}