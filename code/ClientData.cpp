#include "./hpp/ClientData.hpp"

Client::Client() :  _responseSent(0), _client_fd(-1), _server_fd(-1), _headersParsed(false),
        _bodyType(BodyType::None), _contentLength(0), _bodyPos(0),
        _bodySize(0), _requestEnd(0), _requestErrorCode(0), _closeAfterResoinse(false) {
    // std::cout << "Client: " << _client_fd << ", Server: " << _server_fd << std::endl;
};

Client::Client(int client_fd, int server_fd) :  _responseSent(0), _client_fd(client_fd), _server_fd(server_fd), 
        _headersParsed(false), _bodyType(BodyType::None), _contentLength(0), _bodyPos(0),
        _bodySize(0), _requestEnd(0), _requestErrorCode(0), _closeAfterResoinse(false) {
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

void Client::consumeRequest() {

    if (_requestEnd > _requestBuffer.size())
        _requestEnd = _requestBuffer.size();

    _requestBuffer.erase(0, _requestEnd);
    _headersParsed = false;
    _bodyType = BodyType::None;
    _contentLength = 0;
    _bodyPos = 0;
    _bodySize = 0;
    _requestEnd = 0;
    _requestErrorCode = 0;
    _closeAfterResoinse = false;

    _request = HTTPRequest();
};


void Client::setClientRequest(const HTTPRequest& req) {
    _request = req;
};

void Client::setResponseBuffer(const std::string& response) {
    _responseBuffer = response;
    _responseSent = 0;
};

void Client::setResponseSent(size_t sent) {
    _responseSent = sent;
};

void Client::setCloseAfterResponse(bool value) {
    _closeAfterResoinse = value;
};

bool Client::getCloseAfterReponse() const { return _closeAfterResoinse; };
size_t Client::getResponseSent() const { return _responseSent; };
const std::string& Client::getResponseBuffer() const { return _responseBuffer; };
size_t Client::getBodyPos() const { return _bodyPos; };
size_t Client::getBodySize() const { return _bodySize; };
int Client::getClientFd() const { return _client_fd; };
int Client::getServerFd() const { return _server_fd; };
const std::string& Client::getRequestBuffer() const { return _requestBuffer; };
const HTTPRequest& Client::getRequest() const { return _request; };
size_t Client::getRequestEnd() const { return _requestEnd; };
int Client::getRequestErrorCode() const { return _requestErrorCode; };

void Client::clearResponse() {
    _responseBuffer.clear();
    _responseSent = 0;
};
