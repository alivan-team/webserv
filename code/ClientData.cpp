#include "./hpp/ClientData.hpp"
#include <iostream>

Client::Client() : _client_fd(-1), _server_fd(-1) {
    std::cout << "Client: " << _client_fd << ", Server: " << _server_fd << std::endl;
};

Client::Client(int clinet_fd, int server_fd) : _client_fd(clinet_fd), _server_fd(server_fd) {
    std::cout << "Client: " << _client_fd << ", Server: " << _server_fd << std::endl;
};

void Client::appendToRequestBuffer(const char* buffer, size_t bytes) {

    _requestBuffer.append(buffer, bytes);    
};

bool Client::hasCompleteHeaders() const {
    return _requestBuffer.find("\r\n\r\n") != std::string::npos;
};

bool Client::hasCompleteRequest()  {
    // Multiple Content-Length headers need validation
    // if value == -1 == 400 Bad request
    // Header names are case-insensitive -> 
    //  You search the entire buffer -> i should search only the header. and not the body too -> that is wrong


// bool Client::hasCompleteRequest()
// {
//     size_t headerEnd = _requestBuffer.find("\r\n\r\n");

//     if (headerEnd == std::string::npos)
//         return false;

//     // Parse only:
//     // _requestBuffer.substr(0, headerEnd + 2)

//     // 1. Validate Transfer-Encoding
//     // 2. Find and validate all Content-Length headers
//     // 3. Reject Transfer-Encoding + Content-Length
//     // 4. If chunked, check whether the terminating zero chunk arrived
//     // 5. If Content-Length, wait for that many bytes
//     // 6. Otherwise, body length is zero

//     return true;
// }


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

    _bodyPos = headerEnd + 4;
    _bodySize = _requestBuffer.size() - _bodyPos;

    return _bodySize >= contentLength;
};

// Client::RequestState Client::getRequestState()
// {
//     const size_t headerEnd = _requestBuffer.find("\r\n\r\n");

//     if (headerEnd == std::string::npos)
//         return RequestState::Incomplete;

//     const std::string headers =
//         _requestBuffer.substr(0, headerEnd);

//     if (hasChunkedTransferEncoding(headers))
//         return getChunkedRequestState(headerEnd + 4);

//     size_t contentLength = 0;
//     bool hasLength = false;

//     if (!extractContentLength(headers, contentLength, hasLength))
//         return RequestState::BadRequest;

//     _bodyPos = headerEnd + 4;

//     if (!hasLength)
//     {
//         _completeRequestSize = _bodyPos;
//         return RequestState::Complete;
//     }

//     if (contentLength > _maximumBodySize)
//         return RequestState::BodyTooLarge;

//     const size_t receivedBodySize =
//         _requestBuffer.size() - _bodyPos;

//     if (receivedBodySize < contentLength)
//         return RequestState::Incomplete;

//     _bodySize = contentLength;
//     _completeRequestSize = _bodyPos + contentLength;

//     return RequestState::Complete;
// }
        
void Client::clearRequestBuffer() {
    _requestBuffer.clear();
};

void Client::setClientRequest(const HTTPRequest& req) {
    _request = req;
};

std::string Client::getFullBodyRequest() const {
    return _requestBuffer.substr(_bodyPos, _bodySize);
};

std::string Client::getPartBodyRequest(size_t start, size_t length) const {
    
    if (start < 0 && start + length > _bodySize) {
        length = _bodySize - start;
    }
    return _requestBuffer.substr(start, length);

};

size_t Client::getBodyPos() const { return _bodyPos; };
size_t Client::getBodySize() const { return _bodySize; };
int Client::getClientFd() const { return _client_fd; };
int Client::getServerFd() const { return _server_fd; };
const std::string& Client::getRequestBuffer() const { return _requestBuffer; };
const HTTPRequest& Client::getRequest() const { return _request; };

