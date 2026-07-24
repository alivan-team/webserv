
#include "./hpp/ServerManager.hpp"
#include "./hpp/HTTPResponseBuild.hpp"
#include "./hpp/HTTPRequestParser.hpp"

#include "./hpp/printDebug.hpp"

// ServerManager::ServerManager() {};

const std::map<int, ServerConfig>& ServerManager::getServerManager() const {
    return _serversMap;
};

void ServerManager::acceptNewClient(int serverFd) {

    int newClientFd = accept(serverFd, NULL, NULL);

    if (newClientFd < 0)
        return ;

    setNonBlocking(newClientFd);

    pollfd client_poll;
    client_poll.fd = newClientFd;
    client_poll.events = POLLIN;
    client_poll.revents = 0;

    _pollfds.push_back(client_poll);
    _clients[newClientFd] = Client(newClientFd, serverFd);

    std::cout << "New client: fd " << newClientFd << "\n";
};

void ServerManager::readClientData(size_t index) {

    // Header complete?
    // |
    // +-- Transfer-Encoding: chunked ?
    // |         |
    // |         +-- Yes -> parse chunks until the 0-sized chunk.
    // |
    // +-- Content-Length ?
    // |         |
    // |         +-- Yes -> wait until exactly that many body bytes arrive.
    // |
    // +-- Neither?
    //           |
    //           +-- Request has a zero-length body.

    int clientFd = _pollfds[index].fd;

    char buffer[4096];
    std::memset(buffer, 0, sizeof(buffer));

    int bytes = recv(clientFd, buffer, sizeof(buffer) - 1, 0);

    if (bytes == 0) {

        std::cout << "client disconnected fd: " << std::endl;
        close(clientFd);
        _clients.erase(clientFd);
        _pollfds.erase(_pollfds.begin() + index);
        return ;
    } else if (bytes < 0) {

        // std::cout << "Client disconnected fd: " << clientFd << std::endl;

        if (errno == EAGAIN || errno == EWOULDBLOCK) {
            return;
        }
        std::cerr << "recv() failed" << std::endl;
        close(clientFd);
        _clients.erase(clientFd);
        _pollfds.erase(_pollfds.begin() + index);
        return;
    }

    Client& client = _clients.at(clientFd);
    client.appendToRequestBuffer(buffer, static_cast<size_t>(bytes));

    // -> IMPORTANT Check for POST:  Transfer-Encoding: chunked + Content-length
    // in the recieveing we can NOT have Content-length and have Transfer-Encoding... so the 
    // checkRequestState() must be addapted to this... otherwise 411 Length Requierd.
    
    RequestState state = client.checkRequestState();

    if (state == RequestState::Incomplete)
        return ;
    if (state == RequestState::BadRequest) {

        // std::cout << "\n\n BadRequest Here : " << std::endl;
        
        const ServerConfig& serverConfig = getClientServerManager(client.getServerFd());
        
        int errorCode = client.getRequestErrorCode();

        if (errorCode == 0)
            errorCode = 400;

        HTTPRequest errorRequest;
        errorRequest.setVersion("HTTP/1.1");

        HTTPResponse errorResponse = HTTPResponseBuild::makeErrorResponse(400, errorRequest, serverConfig);

        std::string response = errorResponse.toString(errorResponse);

        send(clientFd, response.c_str(), response.size(), 0);
        
        // client.clearRequestBuffer();
        close(clientFd);
        _clients.erase(clientFd);
        _pollfds.erase(_pollfds.begin() + index);
        return ;
    }

    // HTTP REQUST PARSER 
	// std::cout << "HTTPParser ============================\n";
    // HTTPRequest request = HTTPRequestParser().parse(client.getRequestBuffer());
	// printDebug("HTTPRequestParser", request);
	// std::cout << "~HTTPParser ============================\n";
	client.setClientRequest(HTTPRequestParser().parse(client.getRequestBuffer()));

    // HTTP RESPONSE BUILD 
    // ServerConfig has multible servers and I need to connect the Client to the SC.
    // HTTP RESPONSE 
    // later ClassResponse will be changed to response
    HTTPResponse ClassResponse = HTTPResponseBuild::build(client.getRequest(), getClientServerManager(client.getServerFd()));


    // ALL under is default. 
    // std::cout << "~~~~~~ REQUEST ~~~~~~ \n\t client.getRequestBuffer() \n\t -- from fd : " << clientFd << " -- \n";
    // std::cout << "~~~~~~ BODY FROM MANAGER ~~~~~~ " << std::endl;
    // std::cout << ClassResponse.getHeader() << std::endl;

    // std::string body = "Hello from ServerManager\n";

    
    std::string response = ClassResponse.toString(ClassResponse);
    // "HTTP/1.1 200 OK\r\n"
    // "Content-Type: text/plain\r\n"
    // "Content-Length: " + std::to_string(body.size()) + "\r\n"
    // "\r\n" +
    // body;

    // std::string res = response;
    // std::cout << "~~~~~~ RESPONSE ~~~~~~ \n\t" << res << " ----- \n";

    
    send(clientFd, response.c_str(), response.size(), 0);
    
    // TODO: IMPORTANT TO CHECK LATER !!!!!!!!!!!!!!!!!!!
    // Connection: keep-alive
    // It means:
    // After sending the response, do not close the client socket yet.
    // The browser may send another request on the same connection.
    // if response has Connection: keep-alive
    //     keep client fd inside poll()
    //     clear request buffer after complete request
    // else
    //     close(client_fd)
    //     remove from poll()
    // Important: after one full request is handled, clear only the processed request from the client buffer.
    // For HTTP/1.1, keep-alive is the default unless the client sends:
    // Connection: close
    // bool keepAlive = request.getVersion() == "HTTP/1.1"
    //           && request.getHeader("Connection") != "close";

    client.clearRequestBuffer();
    close(clientFd);
    _clients.erase(clientFd);
    _pollfds.erase(_pollfds.begin() + index);
};

bool ServerManager::isServerSocket(int fd) const
{
    for (size_t i = 0; i < _serverSockets.size(); ++i)
    {
        if (_serverSockets[i] == fd)
            return true;
    }
    return false;
}

void ServerManager::initialize(const std::vector<ServerConfig>& servers) {

    if(servers.empty())
        throw std::runtime_error("No servers configured");
    
    for (size_t i = 0; i < servers.size(); i++) {
        int serverFd = createListeningSockets(servers[i]);
        _serversMap[serverFd] = servers[i];
    }
};

void ServerManager::run() {

    while (true) {
        
        int ready = poll(_pollfds.data(), _pollfds.size(), -1);
        if (ready < 0)
            throw std::runtime_error("poll() failed");

        for (size_t i = 0; i < _pollfds.size(); i++) {

            if (_pollfds[i].revents & POLLIN) {

                if (isServerSocket(_pollfds[i].fd)) {
                    acceptNewClient(_pollfds[i].fd);
                } else {
                    readClientData(i);
                }
            }
        }
    }
};

void ServerManager::setNonBlocking(int fd)
{
    int flags = fcntl(fd, F_GETFL, 0);

    if (flags < 0)
        throw std::runtime_error("fcntl(F_GETFL) failed");

    if (fcntl(fd, F_SETFL, flags | O_NONBLOCK) < 0)
        throw std::runtime_error("fcntl(F_SETFL) failed");
}

int ServerManager::createListeningSockets(const ServerConfig& server) {

    int server_fd = socket(AF_INET, SOCK_STREAM, 0);

    if (server_fd < 0) 
        throw std::runtime_error("socket() failed");
    
    int opt = 1;

    if (setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt)) < 0) {
        close(server_fd);
        throw std::runtime_error("setsockopt() failed");
    }

    sockaddr_in addr;
    std::memset(&addr, 0, sizeof(addr));
    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = INADDR_ANY;

    int port = server.getPort();

    addr.sin_port = htons(port);

    if (bind(server_fd, reinterpret_cast<sockaddr*>(&addr), sizeof(addr)) < 0) {
        close(server_fd);
        throw std::runtime_error("bind() failed");
    }

    if (listen(server_fd, 128) < 0) { 
        close(server_fd);
        throw std::runtime_error("listen() failed");
    }

    setNonBlocking(server_fd);

    // server.setServerConfFD(server_fd);

    _serverSockets.push_back(server_fd);

    pollfd server_poll;
    server_poll.fd = server_fd;
    server_poll.events = POLLIN;
    server_poll.revents = 0;

    _pollfds.push_back(server_poll);

    std::cout << "Listening on port: " << port << std::endl;

    return server_fd;
};

const ServerConfig& ServerManager::getClientServerManager(int serverIndex) const {

    std::map<int, ServerConfig>::const_iterator it = _serversMap.find(serverIndex);

    if (it == _serversMap.end())
        throw std::runtime_error("Server configuration not found.");

    return it->second;
};
