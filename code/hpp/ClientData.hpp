#ifndef CLIENTDATA_HPP
#define CLIENTDATA_HPP

#include <string>
#include <cstdlib>
#include <cstddef>
#include <cctype>
#include <cstring>
#include <sstream>
#include <iostream>
#include <limits>
#include "HTTPRequest.hpp"
#include "HTTPResponse.hpp"
#include "./HelperFunctions.hpp"

enum class RequestState {
    Incomplete,
    Complete,
    BadRequest
};

enum class BodyType {
    None,
    ContentLength,
    Chunked
};

class Client {

    private:
        std::string _requestBuffer;
        std::string _responseBuffer;
        
        std::string _body;
        
        size_t _responseSent;
        int _client_fd;
        int _server_fd;
        bool _headersParsed;
        BodyType _bodyType;
        size_t _contentLength;
        size_t _bodyPos;
        size_t _bodySize;
        size_t _requestEnd;
        int _requestErrorCode;
        HTTPRequest _request;
        bool _closeAfterResoinse;
        // HTTPResponse _response;
        
        bool parseContentLength(const std::string& value, size_t& result) const;
        bool parseHexSize(const std::string& value, size_t& result) const;
        RequestState checkChunkedBody(size_t bodyStart, size_t& requestEnd, size_t& decodedBodySize, size_t maxBodySize);
        std::string trim(const std::string& value) const;
        // std::string toLower(const std::string& value) const;
        RequestState parseHeaders();
        RequestState checkChunkedRequestBody(size_t maxBodySize);
        RequestState checkContentLengthBody();
        RequestState setRequestError(int errorCode);

    public:
                    
        Client();
        Client(int client_fd, int server_fd);

        void appendToRequestBuffer(const char* buffer, size_t bytes);
        RequestState checkRequestState(size_t maxBodySize);
        void clearRequestBuffer();
        void consumeRequest();
        void setClientRequest(const HTTPRequest& req);
        void setResponseBuffer(const std::string& response);
        void setResponseSent(size_t sent);
        void setCloseAfterResponse(bool value);
        bool getCloseAfterReponse() const;
        size_t getResponseSent() const;
        const std::string& getResponseBuffer() const;
        size_t getBodyPos() const;
        size_t getBodySize() const;
		BodyType getBodyType() const;
        int getClientFd() const;
        int getServerFd() const;
        const std::string& getRequestBuffer() const;
        const HTTPRequest& getRequest() const;
        int     getRequestErrorCode() const;
        size_t getRequestEnd() const;
        void clearResponse();
		bool decodeChunkedBody();
};

#endif