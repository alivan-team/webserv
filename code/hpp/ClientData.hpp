#ifndef CLIENTDATA_HPP
#define CLIENTDATA_HPP

#include <string>
#include <cstdlib>
#include <cstddef>
#include <cctype>
#include <sstream>
#include <limits>
#include "HTTPRequest.hpp"
#include "HTTPResponse.hpp"

enum class RequestState
{
    Incomplete,
    Complete,
    BadRequest
};

enum class BodyType
{
    None,
    ContentLength,
    Chunked
};

class Client {

        private:
                std::string _requestBuffer;
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
                // HTTPResponse _response;
                
                bool parseContentLength(const std::string& value, size_t& result) const;
                bool parseHexSize(const std::string& value, size_t& result) const;
                RequestState checkChunkedBody(size_t bodyStart, size_t& requestEnd) const;
                std::string trim(const std::string& value) const;
                std::string toLower(const std::string& value) const;
                RequestState parseHeaders();
                RequestState checkChunkedRequestBody();
                RequestState checkContentLengthBody();
                RequestState setRequestError(int errorCode);

        public:
                        
                Client();
                Client(int clinet_fd, int server_fd);

                void appendToRequestBuffer(const char* buffer, size_t bytes);
                RequestState checkRequestState();
                void clearRequestBuffer();
                void setClientRequest(const HTTPRequest& req);
                size_t getBodyPos() const;
                size_t getBodySize() const;
                int getClientFd() const;
                int getServerFd() const;
                const std::string& getRequestBuffer() const;
                const HTTPRequest& getRequest() const;
                int     getRequestErrorCode() const;

};

#endif