#include "./hpp/ClientData.hpp"

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

RequestState Client::checkChunkedRequestBody(size_t maxBodySize) {
    
    size_t checkedRequestEnd = 0;
    size_t decodedBodySize = 0;

    RequestState chunkedState = checkChunkedBody(_bodyPos, checkedRequestEnd, decodedBodySize, maxBodySize);
    
    if(chunkedState != RequestState::Complete)
        return chunkedState;

    _requestEnd = checkedRequestEnd;
    _bodySize = decodedBodySize; //-> size only info 
    // _bodySize = _requestEnd - _bodyPos; // size info + protocol;

    return RequestState::Complete;
};

// POST /upload HTTP/1.1
// Host: localhost:8080
// Content-Type: text/plain
// Transfer-Encoding: chunked /r/n
// /r/n
// 5/r/n
// hello/r/n
// 3/r/n
// asd/r/n
// /r/n
// GET /upload HTTP/1.1
// Host: localhost:8080
// ...
// decodedBodySize = 8
// _requestEnd - _bodyPos = 24

//  Test chunk : 
//       curl -v -X POST \
//         -H "Content-Type: text/plain" \
//         -H "Transfer-Encoding: chunked" \
//         --data-binary "$(printf 'A%.0s' {1..200})" \
//         http://localhost:8080/upload

// test content_length: 
//       curl -v -X POST \
//         -H "Content-Type: text/plain" \
//         --data-binary "12345678901" \
//         http://localhost:8080/upload


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

RequestState Client::checkChunkedBody(size_t bodyStart, size_t& requestEnd, size_t& decodedBodySize, size_t maxBodySize) {

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

        if (decodedBodySize > maxBodySize)
            return setRequestError(413);
        if (chunkHex > maxBodySize - decodedBodySize)
            return setRequestError(413);
        decodedBodySize += chunkHex;

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

RequestState Client::checkRequestState(size_t maxBodySize)  {

    if (!_headersParsed) {
        RequestState headerState = parseHeaders();
        if (headerState != RequestState::Complete)
            return headerState;
    }

    if (_bodyType == BodyType::ContentLength) {
        if (_contentLength > maxBodySize)
            return setRequestError(413);
        return checkContentLengthBody();
    }

    if (_bodyType == BodyType::Chunked)
        return checkChunkedRequestBody(maxBodySize);
    
    _bodySize = 0;
    _requestEnd = _bodyPos;

    return RequestState::Complete;
};

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

RequestState Client::setRequestError(int errorCode) {
    _requestErrorCode = errorCode;
    return RequestState::BadRequest;
};
