#include "./hpp/ClientData.hpp"
#include <iostream>

Client::Client() : _client_fd(-1), _server_fd(-1), _headersParsed(false),
        _bodyType(BodyType::None), _contentLength(0), _bodyPos(0),
        _bodySize(0), _requestEnd(0), _requestErrorCode(0) {
    // std::cout << "Client: " << _client_fd << ", Server: " << _server_fd << std::endl;
};

Client::Client(int clinet_fd, int server_fd) : _client_fd(clinet_fd), _server_fd(server_fd), 
        _headersParsed(false), _bodyType(BodyType::None), _contentLength(0), _bodyPos(0),
        _bodySize(0), _requestEnd(0), _requestErrorCode(0) {
    // std::cout << "Client: " << _client_fd << ", Server: " << _server_fd << std::endl;
};

void Client::appendToRequestBuffer(const char* buffer, size_t bytes) {

    _requestBuffer.append(buffer, bytes);    
};


RequestState Client::checkRequestState()  {

    if (!_headersParsed) {

        RequestState headerState = parseHeaders();
        if (headerState != RequestState::Complete)
            return headerState;
    }

    if (_bodyType == BodyType::ContentLength) 
        return checkContentLengthBody();

    if (_bodyType == BodyType::Chunked) 
        return checkChunkedRequestBody();
    
    _bodySize = 0;
    _requestEnd = _bodyPos;

    return RequestState::Complete;
};

RequestState Client::checkChunkedBody(size_t bodyStart, size_t& requestEnd) const {

    size_t position = bodyStart;

    while (true) {

        size_t sizeLineEnd = _requestBuffer.find("\r\n", position);
        if (sizeLineEnd == std::string::npos) 
            return RequestState::Incomplete;

        std::string sizeLineStart = _requestBuffer.substr(position, sizeLineEnd - position);
        size_t extentionPosition = sizeLineStart.find(";");
        if (extentionPosition != std::string::npos)
            sizeLineStart = sizeLineStart.substr(0, extentionPosition);

        sizeLineStart = trim(sizeLineStart);
        if(sizeLineStart.empty()) 
            return RequestState::BadRequest;

        size_t chunkHex = 0;

        if (!parseHexSize(sizeLineStart, chunkHex))
            return RequestState::BadRequest;

        position = sizeLineEnd + 2;

        if (chunkHex == 0) {

            if(_requestBuffer.size() < position + 2)
                return RequestState::Incomplete;

            if(_requestBuffer.compare(position, 2, "\r\n") != 0)
                return RequestState::BadRequest;

            requestEnd = position + 2;

            return RequestState::Complete;
        
        }

        if(position > std::numeric_limits<size_t>::max() - chunkHex) 
            return RequestState::BadRequest;

        size_t chunkEnd = position + chunkHex;

        if (chunkEnd > std::numeric_limits<size_t>::max() - 2)
            return RequestState::BadRequest;

        if (_requestBuffer.size() < chunkEnd + 2)
            return RequestState::Incomplete;

        if (_requestBuffer.compare(chunkEnd, 2, "\r\n") != 0)
            return RequestState::BadRequest;

        position = chunkEnd + 2;
    }
}

bool Client::parseHexSize(const std::string& value, size_t& result) const {

    if (value.empty())
        return false;

    try {
        size_t parsedCharacters = 0;

        unsigned long long parsedValue = std::stoull(value, &parsedCharacters, 16);

        if (parsedCharacters != value.size())
            return false;

        if (parsedValue > std::numeric_limits<size_t>::max())
            return false;

        result = static_cast<size_t>(parsedValue);
        return true;

    } catch (const std::invalid_argument&) {
        return false;
    } catch (const std::out_of_range&) {
        return false;
    }
};

// ///////////////////////////////////////////////////////////////////////////////////////////
// ///////////////////////////////////////////////////////////////////////////////////////////
// ///////////////////////////////////////////////////////////////////////////////////////////
// ///////////////////////////////////////////////////////////////////////////////////////////
// /////////////////////////////////// Helper Functions //////////////////////////////////////
// ///////////////////////////////////////////////////////////////////////////////////////////
// ///////////////////////////////////////////////////////////////////////////////////////////
// ///////////////////////////////////////////////////////////////////////////////////////////
// ///////////////////////////////////////////////////////////////////////////////////////////

RequestState Client::setRequestError(int errorCode) {
    _requestErrorCode = errorCode;
    return RequestState::BadRequest;
};

RequestState Client::checkContentLengthBody() {

    if (_requestBuffer.size() < _bodyPos)
        return RequestState::Incomplete;

    size_t receivedBodySize =
        _requestBuffer.size() - _bodyPos;

    if (receivedBodySize < _contentLength)
        return RequestState::Incomplete;

    _bodySize = _contentLength;
    _requestEnd = _bodyPos + _contentLength;

    return RequestState::Complete;
};

RequestState Client::checkChunkedRequestBody() {
    
    size_t checkedRequestEnd = 0;

    RequestState chunkedState = checkChunkedBody(_bodyPos, checkedRequestEnd);

    if(chunkedState == RequestState::Incomplete) 
        return RequestState::Incomplete;

    if(chunkedState == RequestState::BadRequest) 
        return setRequestError(400);

    _requestEnd = checkedRequestEnd;
    _bodySize = _requestEnd - _bodyPos;

    return RequestState::Complete;
};

RequestState Client::parseHeaders() {

    size_t headerEnd = _requestBuffer.find("\r\n\r\n");

    if (headerEnd == std::string::npos) 
        return RequestState::Incomplete;

    _bodyPos = headerEnd + 4;

    std::string headerSection = _requestBuffer.substr(0, headerEnd);
    std::istringstream headerStreamSection(headerSection);
    std::string line;

    if (!getline(headerStreamSection, line))
        return setRequestError(400);

    if (!line.empty() && line[line.size() - 1] == '\r')
        line.erase(line.size() - 1);

    // if the first line is empty ??? can it be ?? 
    if (line.empty())
        return RequestState::BadRequest;

    bool hasContentLength = false;
    bool hasTransferEncoding = false;
    bool isChunked = false;

    size_t parseContntLength = 0;

    while (std::getline(headerStreamSection, line)) {

        if (!line.empty() && line[line.size() - 1] == '\r')
            line.erase(line.size() - 1);

        if(line.empty())
            continue;

        size_t colonmPosition = line.find(":");

        if (colonmPosition == std::string::npos) 
            return setRequestError(400);

        std::string headerName = toLower(trim(line.substr(0, colonmPosition)));
        std::string headerValue = trim(line.substr(colonmPosition + 1));

        if (headerName.empty()) 
            return setRequestError(400);

        if (headerName == "content-length") {

            if (hasContentLength) 
                return setRequestError(400);

            if (!parseContentLength(headerValue, parseContntLength)) 
                return setRequestError(400);

            hasContentLength = true;

        } else if (headerName == "transfer-encoding") {

            if (hasTransferEncoding) 
                return setRequestError(400);

            hasTransferEncoding = true;

            std::string lowerValue = toLower(headerValue);
            if (lowerValue != "chunked") {
                _requestErrorCode = 501;
                return RequestState::BadRequest;
            }
        }
    }

    if (hasContentLength && hasTransferEncoding) 
        return setRequestError(400);

    if (hasTransferEncoding) {
        _bodyType = BodyType::Chunked;
    } else if (hasContentLength) {
        _bodyType = BodyType::ContentLength;
        _contentLength = parseContntLength;
    } else {
        _bodyType = BodyType::None;
        _contentLength = 0;
    }

    _headersParsed = true;

    return RequestState::Complete;
};

std::string Client::trim(const std::string& value) const
{
    size_t start = 0;

    while (start < value.size() && (value[start] == ' ' || value[start] == '\t')) {
        ++start;
    }

    size_t end = value.size();

    while (end > start && (value[end - 1] == ' ' || value[end - 1] == '\t')) {
        --end;
    }

    return value.substr(start, end - start);
}

std::string Client::toLower(const std::string& value) const
{
    std::string result = value;

    for (size_t i = 0; i < result.size(); ++i) {
        result[i] = static_cast<char>(std::tolower(static_cast<unsigned char>(result[i])));
    }

    return result;
}

bool Client::parseContentLength(const std::string& value, size_t& result) const {

    std::string cleanValue = trim(value);

    if (cleanValue.empty())
        return false;

    result = 0;

    for (size_t i = 0; i < cleanValue.size(); ++i) {

        unsigned char character = static_cast<unsigned char>(cleanValue[i]);

        if (!std::isdigit(character))
            return false;

        size_t digit = static_cast<size_t>(cleanValue[i] - '0');

        if (result > (std::numeric_limits<size_t>::max() - digit) / 10) {
            return false;
        }

        result = result * 10 + digit;
    }

    return true;
}
        
void Client::clearRequestBuffer() {
    _requestBuffer.clear();
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
