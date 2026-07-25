#include "./hpp/ClientData.hpp"

Client::Client() : _client_fd(-1), _server_fd(-1), _headersParsed(false),
        _bodyType(BodyType::None), _contentLength(0), _bodyPos(0),
        _bodySize(0), _requestEnd(0), _requestErrorCode(0) {
    // std::cout << "Client: " << _client_fd << ", Server: " << _server_fd << std::endl;
};

Client::Client(int client_fd, int server_fd) : _client_fd(client_fd), _server_fd(server_fd), 
        _headersParsed(false), _bodyType(BodyType::None), _contentLength(0), _bodyPos(0),
        _bodySize(0), _requestEnd(0), _requestErrorCode(0) {
    // std::cout << "Client: " << _client_fd << ", Server: " << _server_fd << std::endl;
};

void Client::appendToRequestBuffer(const char* buffer, size_t bytes) {

    _requestBuffer.append(buffer, bytes);    
};
        
void Client::clearRequestBuffer() {
    _requestBuffer.clear();

    _headersParsed = false;
    _bodyType = BodyType::None;
    _contentLength = 0;
    _bodyPos = 0;
    _bodySize = 0;
    _requestEnd = 0;
    _requestErrorCode = 0;

    _request = HTTPRequest();
};

void Client::setClientRequest(const HTTPRequest& req) {
    _request = req;
};

size_t Client::getBodyPos() const { return _bodyPos; };
size_t Client::getBodySize() const { return _bodySize; };
int Client::getClientFd() const { return _client_fd; };
int Client::getServerFd() const { return _server_fd; };
const std::string& Client::getRequestBuffer() const { return _requestBuffer; };
const HTTPRequest& Client::getRequest() const { return _request; };
int Client::getRequestErrorCode() const { return _requestErrorCode; };
