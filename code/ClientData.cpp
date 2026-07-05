#include "./hpp/ClientData.hpp"

Client::Client() : _client_fd(-1), _server_fd(-1) {};

Client::Client(int clinet_fd, int server_fd) : _client_fd(clinet_fd), _server_fd(server_fd) {};

void Client::appendToRequestBuffer(const char* buffer, size_t bytes) {

    _requestBuffer.append(buffer, bytes);    
};

bool Client::hasCompleteHeaders() const {
    return _requestBuffer.find("\r\n\r\n") != std::string::npos;
};

bool Client::hasCompleteRequest() const {
    size_t headerEnd = _requestBuffer.find("\r\n\r\n");

    if (headerEnd == std::string::npos)
        return false;

    size_t contentLengthPos = _requestBuffer.find("Content-Length:");

    if (contentLengthPos == std::string::npos)
        return true; // GET usually has no body

    size_t valueStart = contentLengthPos + std::string("Content-Length:").size();
    size_t valueEnd = _requestBuffer.find("\r\n", valueStart);

    if (valueEnd == std::string::npos)
        return false;

    std::string value = _requestBuffer.substr(valueStart, valueEnd - valueStart);
    size_t contentLength = std::atoi(value.c_str());
    // maybe validate if its a negative value? 

    size_t bodyStart = headerEnd + 4;
    size_t bodySize = _requestBuffer.size() - bodyStart;

    return bodySize >= contentLength;
};
        
void Client::clearRequestBuffer() {};


int Client::getClientFd() const { return _client_fd; };
int Client::getServerFd() const { return _server_fd; };
const std::string& Client::getRequestBuffer() const { return _requestBuffer; };
