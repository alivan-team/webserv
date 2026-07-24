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

// ///////////////////////////////////////////////////////////////////////////////////////////
// ///////////////////////////////////////////////////////////////////////////////////////////
// ///////////////////////////////////////////////////////////////////////////////////////////
// ///////////////////////////////////////////////////////////////////////////////////////////
// ///////////////////////////////////////////////////////////////////////////////////////////
// ///////////////////////////////////////////////////////////////////////////////////////////
// ///////////////////////////////////////////////////////////////////////////////////////////
// ///////////////////////////////////////////////////////////////////////////////////////////


RequestState Client::checkRequestState()  {

    // std::cout << "\t 1 : " << std::endl;

    _bodyPos = 0;
    _bodySize = 0;
    _requestEnd = 0;
    _requestErrorCode = 0;
        
    size_t headerEnd = _requestBuffer.find("\r\n\r\n");

    if (headerEnd == std::string::npos) 
        return RequestState::Incomplete;

    _bodyPos = headerEnd + 4;

    // || parse only the HEADER from the request ->
    std::string headerSection = _requestBuffer.substr(0, headerEnd);
    std::istringstream headerStreamSection(headerSection);
    std::string line;

    if (!getline(headerStreamSection, line)) {
        _requestErrorCode = 400;
        return RequestState::BadRequest;
    }

    if (!line.empty() && line[line.size() - 1] == '\r')
        line.erase(line.size() - 1);

    bool hasContentLength = false;
    size_t contentLength = 0;
    bool hasTransferEncoding = false;
    bool isChunked = false;

    // std::cout << "\t HEADER : " << std::endl;
    while (std::getline(headerStreamSection, line)) {

        // std::cout << "\t\t line : " << line << std::endl;


        if (!line.empty() && line[line.size() - 1] == '\r')
            line.erase(line.size() - 1);

        if(line.empty())
            continue;

        size_t colonmPosition = line.find(":");

        if (colonmPosition == std::string::npos) {
            _requestErrorCode = 400;
            return RequestState::BadRequest;
        }

        std::string headerName = toLower(trim(line.substr(0, colonmPosition)));
        std::string headerValue = trim(line.substr(colonmPosition + 1));
        // std::cout << "headerName : " << headerName << std::endl;
        // std::cout << "\t headerValue : " << headerValue << std::endl;

        if (headerName.empty()) {
            _requestErrorCode = 400;
            return RequestState::BadRequest;
        }

        // std::cout << "\n\t\t hasContentLength : " << hasContentLength
        //         << "\n\t\t hasTransferEncoding : " << hasTransferEncoding << std::endl;


        if (headerName == "content-length") {

            if (hasContentLength) {
                _requestErrorCode = 400;
                return RequestState::BadRequest;
            }

            size_t parseContntLength = 0;

            if (!parseContentLength(headerValue, parseContntLength)) {
                _requestErrorCode = 400;
                return RequestState::BadRequest;
            }

            hasContentLength = true;
            contentLength = parseContntLength;

        } else if (headerName == "transfer-encoding") {

            if (hasTransferEncoding) {
                _requestErrorCode = 400;
                // std::cout << "\t\t\t\t _requestErrorCode : " << _requestErrorCode << std::endl;
                return RequestState::BadRequest;
            }

            hasTransferEncoding = true;

            std::string lowerValue = toLower(headerValue);
            if (lowerValue == "chunked") {
                isChunked = true;
            } else {
                _requestErrorCode = 501;
                return RequestState::BadRequest;
            }
        }

        // std::cout << "-> headerName : " << headerName << std::endl;
        // std::cout << "-> headerValue : " << headerValue << std::endl;
    }
    // <- parse only the HEADER from the request ||



    if (hasContentLength && hasTransferEncoding) {
        _requestErrorCode = 400;
        return RequestState::BadRequest;
    }

    if (isChunked) {
        // std::cout << "\n\n isChunked : " << isChunked << std::endl;

        size_t checkedRequestEnd = 0;

        RequestState chunkedState = checkChunkedBody(_bodyPos, checkedRequestEnd);

        if(chunkedState == RequestState::Incomplete) 
            return RequestState::Incomplete;
        if(chunkedState == RequestState::BadRequest) {
            _requestErrorCode = 400;
            return RequestState::BadRequest;
        }

        _requestEnd = checkedRequestEnd;
        _bodySize = _requestEnd - _bodyPos;

        return RequestState::Complete;
    }

    if (hasContentLength) {
        if (_requestBuffer.size() < _bodyPos)
            return RequestState::Incomplete;

        size_t receivedBodySize =
            _requestBuffer.size() - _bodyPos;

        if (receivedBodySize < contentLength)
            return RequestState::Incomplete;

        _bodySize = contentLength;
        _requestEnd = _bodyPos + contentLength;

        return RequestState::Complete;
    }
    
    _bodySize = 0;
    _requestEnd = _bodyPos;

    return RequestState::Complete;
};

// 5\r\n
// hello\r\n
// 6\r\n
//  world\r\n
// 0\r\n
// \r\n
// Chunked can have extentions -> we should ignore it. 
// 5;name=value\r\n


RequestState Client::checkChunkedBody(size_t bodyStart, size_t& requestEnd) const {

    size_t position = bodyStart;
    // int i = 0;

    while (true) {

        // std::cout << "loop in chunked " << std::endl;

        size_t sizeLineEnd = _requestBuffer.find("\r\n", position);
        if (sizeLineEnd == std::string::npos) 
            return RequestState::Incomplete;

        std::string sizeLineStart = _requestBuffer.substr(position, sizeLineEnd - position);
    
        // std::cout << " --- sizeLineStart : " << sizeLineStart << std::endl;


        size_t extentionPosition = sizeLineStart.find(";");
        if (extentionPosition != std::string::npos)
            sizeLineStart = sizeLineStart.substr(0, extentionPosition);

        sizeLineStart = trim(sizeLineStart);
        if(sizeLineStart.empty()) 
            return RequestState::BadRequest;

        // std::cout << " --- sizeLineStart after trim : " << sizeLineStart << std::endl;

        size_t chunkHex = 0;

        if (!parseHexSize(sizeLineStart, chunkHex))
            return RequestState::BadRequest;

        // std::cout << " --- Converted chunkHex to decimal -> " << chunkHex << std::endl;

        position = sizeLineEnd + 2;

        // std::cout << " --- position = sizeLineEnd + 2; -> " << position << std::endl;


        if (chunkHex == 0) {

            // std::cout << " Complete " << std::endl;

            if(_requestBuffer.size() < position + 2)
                return RequestState::Incomplete;

            if(_requestBuffer.compare(position, 2, "\r\n") != 0)
                return RequestState::BadRequest;

            requestEnd = position + 2;

            return RequestState::Complete;
        
        }

        if(position > std::numeric_limits<size_t>::max() - chunkHex) {
            // std::cout << " 1 "  << std::endl;
            return RequestState::BadRequest;
        }

        size_t chunkEnd = position + chunkHex;

        if (chunkEnd > std::numeric_limits<size_t>::max() - 2) {
            // std::cout << " 2 "  << std::endl;
            return RequestState::BadRequest;
        }

        if (_requestBuffer.size() < chunkEnd + 2) {
            // std::cout << " 3 "  << std::endl;
            return RequestState::Incomplete;
        }

        if (_requestBuffer.compare(chunkEnd, 2, "\r\n") != 0) {
            // std::cout << " 4 "  << std::endl;
            return RequestState::BadRequest;
        }

        position = chunkEnd + 2;
        // if(i == 10)
        //     return RequestState::Complete;
        
        // i++;
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

// ///////////////////////////////////////////////////////////////////////////////////////////
// ///////////////////////////////////////////////////////////////////////////////////////////
// ///////////////////////////////////////////////////////////////////////////////////////////
// ///////////////////////////////////////////////////////////////////////////////////////////
// ///////////////////////////////////////////////////////////////////////////////////////////
// ///////////////////////////////////////////////////////////////////////////////////////////
// ///////////////////////////////////////////////////////////////////////////////////////////
// ///////////////////////////////////////////////////////////////////////////////////////////











        
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
int Client::getRequestErrorCode() const { return _requestErrorCode; };
